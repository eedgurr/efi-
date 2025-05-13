#include "obd2_core.h"
#include "j2534_interface.h"

/* KWP2000 Constants */
#define KWP_HEADER_LENGTH     4
#define KWP_MAX_LENGTH       255
#define KWP_BITRATE        10400

/* KWP2000 Service IDs */
#define KWP_SID_START_DIAGNOSTIC   0x10
#define KWP_SID_READ_DATA          0x22
#define KWP_SID_WRITE_DATA         0x2E
#define KWP_SID_CLEAR_DIAGNOSTIC   0x14
#define KWP_SID_READ_ERRORS        0x18

static int kwp_init(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing KWP2000 protocol");
    
    /* Configure J2534 for KWP2000 */
    if (J2534_Connect(PROTOCOL_ISO_14230_4, 0, KWP_BITRATE) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize KWP2000");
        return -1;
    }
    
    /* Start diagnostic session */
    uint8_t start_diag[] = {KWP_SID_START_DIAGNOSTIC, 0x85};
    uint32_t msg_count = 1;
    
    if (J2534_WriteMsgs(0, start_diag, msg_count, 1000) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to start KWP2000 diagnostic session");
        return -1;
    }
    
    return 0;
}

int kwp_send_request(uint8_t service_id, const uint8_t* data, size_t length) {
    uint8_t buffer[KWP_MAX_LENGTH];
    if (length > (KWP_MAX_LENGTH - KWP_HEADER_LENGTH)) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Message too long for KWP2000");
        return -1;
    }
    
    /* Format KWP message */
    buffer[0] = service_id;
    memcpy(&buffer[1], data, length);
    
    uint32_t msg_count = 1;
    return J2534_WriteMsgs(0, buffer, msg_count, 1000);
}

int kwp_receive_response(uint8_t* data, size_t* length) {
    uint8_t buffer[KWP_MAX_LENGTH];
    uint32_t msg_count = 1;
    
    if (J2534_ReadMsgs(0, buffer, &msg_count, 1000) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive KWP2000 response");
        return -1;
    }
    
    /* Check response format */
    if (buffer[0] != (0x40 | buffer[0])) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Invalid KWP2000 response");
        return -1;
    }
    
    *length = msg_count - 1;
    memcpy(data, &buffer[1], *length);
    
    return 0;
}
