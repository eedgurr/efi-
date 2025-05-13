#include "obd2_core.h"
#include "j2534_interface.h"

/* J1850 Protocol Constants */
#define J1850_HEADER_LENGTH    3
#define J1850_MAX_LENGTH      11
#define J1850_PWM_BITRATE    41600
#define J1850_VPW_BITRATE    10400

/* J1850 Message Types */
#define J1850_TYPE_REQUEST    0x6A
#define J1850_TYPE_RESPONSE   0x6B

static int j1850_init(uint8_t protocol) {
    uint32_t bitrate = (protocol == PROTOCOL_SAE_J1850_PWM) ? 
                       J1850_PWM_BITRATE : J1850_VPW_BITRATE;
    
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing J1850 protocol: %s", 
                (protocol == PROTOCOL_SAE_J1850_PWM) ? "PWM" : "VPW");
    
    /* Configure J2534 for J1850 */
    if (J2534_Connect(protocol, 0, bitrate) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize J1850");
        return -1;
    }
    
    return 0;
}

int j1850_send_message(const uint8_t* data, size_t length) {
    uint8_t buffer[J1850_MAX_LENGTH];
    if (length > (J1850_MAX_LENGTH - J1850_HEADER_LENGTH)) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Message too long for J1850");
        return -1;
    }
    
    /* Format J1850 message */
    buffer[0] = J1850_TYPE_REQUEST;
    buffer[1] = 0x6A;  /* Target address (ECU) */
    buffer[2] = 0xF1;  /* Source address (tool) */
    memcpy(&buffer[3], data, length);
    
    uint32_t msg_count = 1;
    return J2534_WriteMsgs(0, buffer, msg_count, 1000);
}

int j1850_receive_message(uint8_t* data, size_t* length) {
    uint8_t buffer[J1850_MAX_LENGTH];
    uint32_t msg_count = 1;
    
    if (J2534_ReadMsgs(0, buffer, &msg_count, 1000) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive J1850 message");
        return -1;
    }
    
    if (buffer[0] != J1850_TYPE_RESPONSE) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Invalid J1850 response type");
        return -1;
    }
    
    *length = msg_count - J1850_HEADER_LENGTH;
    memcpy(data, &buffer[J1850_HEADER_LENGTH], *length);
    
    return 0;
}
