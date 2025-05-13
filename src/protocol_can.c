#include "obd2_core.h"
#include "j2534_interface.h"

/* CAN Protocol Constants */
#define CAN_STD_ID_MASK    0x7FF
#define CAN_EXT_ID_MASK    0x1FFFFFFF
#define CAN_MAX_DLC        8

/* CAN Frame Types */
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN_MAX_DLC];
    uint8_t is_extended;
    uint8_t is_remote;
} CANFrame;

/* CAN Protocol Implementation */
static int can_init(uint32_t baudrate, uint8_t extended_id) {
    uint32_t flags = extended_id ? J2534_CAN_29BIT_ID : 0;
    
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing CAN protocol: %s ID, %d baud",
                extended_id ? "Extended" : "Standard", baudrate);
    
    if (J2534_Connect(J2534_PROTOCOL_CAN, flags, baudrate) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize CAN");
        return -1;
    }
    
    return 0;
}

int can_send_frame(const CANFrame* frame) {
    if (!frame) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "NULL frame pointer");
        return -1;
    }
    
    uint8_t buffer[sizeof(CANFrame)];
    uint32_t msg_count = 1;
    
    /* Format CAN frame */
    memcpy(buffer, frame, sizeof(CANFrame));
    
    return J2534_WriteMsgs(0, buffer, msg_count, 1000);
}

int can_receive_frame(CANFrame* frame, uint32_t timeout_ms) {
    if (!frame) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "NULL frame pointer");
        return -1;
    }
    
    uint8_t buffer[sizeof(CANFrame)];
    uint32_t msg_count = 1;
    
    if (J2534_ReadMsgs(0, buffer, &msg_count, timeout_ms) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive CAN frame");
        return -1;
    }
    
    memcpy(frame, buffer, sizeof(CANFrame));
    return 0;
}

/* CAN Filter Configuration */
int can_set_filter(uint32_t id, uint32_t mask, uint8_t extended) {
    SCONFIG_LIST filter;
    filter.Parameter = extended ? J2534_CAN_29BIT_ID : 0;
    filter.Value = id & (extended ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);
    
    return J2534_IoctlControl(0, J2534_IOCTL_SET_CONFIG, &filter, NULL);
}

/* CAN Error Detection */
int can_check_bus_status(void) {
    SCONFIG_LIST status;
    status.Parameter = 0x00; /* Get bus status */
    
    if (J2534_IoctlControl(0, J2534_IOCTL_GET_CONFIG, NULL, &status) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to get CAN bus status");
        return -1;
    }
    
    return status.Value == 0 ? 0 : -1;
}

/* ISO-TP (ISO 15765-2) Support */
#define ISO_TP_MAX_LENGTH 4095

int can_iso_tp_send(uint32_t id, const uint8_t* data, size_t length) {
    if (length > ISO_TP_MAX_LENGTH) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Message too long for ISO-TP");
        return -1;
    }
    
    /* Format ISO-TP message */
    CANFrame frame;
    frame.id = id;
    frame.is_extended = (id > CAN_STD_ID_MASK) ? 1 : 0;
    frame.is_remote = 0;
    
    if (length <= 7) {
        /* Single frame */
        frame.dlc = length + 1;
        frame.data[0] = length;
        memcpy(&frame.data[1], data, length);
        return can_send_frame(&frame);
    } else {
        /* Multi-frame transmission */
        uint16_t remaining = length;
        uint8_t index = 0;
        
        /* First frame */
        frame.dlc = 8;
        frame.data[0] = 0x10 | ((length >> 8) & 0x0F);
        frame.data[1] = length & 0xFF;
        memcpy(&frame.data[2], data, 6);
        
        if (can_send_frame(&frame) != 0) {
            return -1;
        }
        
        /* Consecutive frames */
        remaining -= 6;
        index = 6;
        uint8_t sequence = 1;
        
        while (remaining > 0) {
            size_t block_size = remaining > 7 ? 7 : remaining;
            frame.data[0] = 0x20 | (sequence & 0x0F);
            memcpy(&frame.data[1], &data[index], block_size);
            frame.dlc = block_size + 1;
            
            if (can_send_frame(&frame) != 0) {
                return -1;
            }
            
            remaining -= block_size;
            index += block_size;
            sequence = (sequence + 1) & 0x0F;
        }
    }
    
    return 0;
}
