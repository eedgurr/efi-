#include "obd2_core.h"
#include "j2534_interface.h"
#include <string.h>

/* Diagnostic Trouble Code Structure */
typedef struct {
    char code[5];           /* DTC code (Pxxxx, Cxxxx, etc.) */
    uint16_t raw_code;      /* Raw binary code */
    char description[100];  /* DTC description */
    uint8_t status;         /* Current/Pending/History */
    uint32_t timestamp;     /* When the DTC was set */
} DTCInfo;

/* Freeze Frame Data Structure */
typedef struct {
    uint16_t dtc;          /* DTC that triggered the freeze frame */
    uint8_t pid;           /* Parameter ID */
    uint8_t data[4];       /* Data bytes */
    float value;           /* Calculated value */
} FreezeFrame;

/* Read DTCs with enhanced information */
int diag_read_dtcs(DTCInfo* dtcs, size_t* count) {
    uint8_t response[256];
    uint32_t msg_count = 1;
    size_t dtc_count = 0;
    
    /* Request DTCs (Mode 03) */
    PID_Request req = {OBD_MODE_READ_TROUBLE_CODES, 0x00};
    if (obd2_send_request(&req) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to request DTCs");
        return -1;
    }
    
    /* Receive response */
    if (J2534_ReadMsgs(0, response, &msg_count, 1000) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive DTC response");
        return -1;
    }
    
    /* Parse DTCs */
    for (size_t i = 0; i < msg_count; i += 2) {
        uint16_t raw_code = (response[i] << 8) | response[i + 1];
        if (raw_code == 0) continue;
        
        DTCInfo* dtc = &dtcs[dtc_count++];
        dtc->raw_code = raw_code;
        
        /* Format DTC code */
        char type = 'P';  /* Powertrain */
        if ((raw_code & 0xC000) == 0x4000) type = 'C';  /* Chassis */
        if ((raw_code & 0xC000) == 0x8000) type = 'B';  /* Body */
        if ((raw_code & 0xC000) == 0xC000) type = 'U';  /* Network */
        
        snprintf(dtc->code, sizeof(dtc->code), "%c%04X", 
                type, raw_code & 0x3FFF);
        
        /* Get DTC status */
        req.mode = 0x07;  /* Get DTC status */
        req.pid = raw_code & 0xFF;
        
        if (obd2_send_request(&req) == 0) {
            PID_Response resp;
            if (obd2_receive_response(&resp) == 0) {
                dtc->status = resp.data[0];
            }
        }
        
        dtc->timestamp = (uint32_t)time(NULL);
    }
    
    *count = dtc_count;
    return 0;
}

/* Read freeze frame data */
int diag_read_freeze_frame(uint16_t dtc, FreezeFrame* data, size_t* count) {
    uint8_t response[256];
    uint32_t msg_count = 1;
    size_t frame_count = 0;
    
    /* Request freeze frame (Mode 02) */
    PID_Request req = {OBD_MODE_SHOW_FREEZE_FRAME, 0x00};
    if (obd2_send_request(&req) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to request freeze frame");
        return -1;
    }
    
    /* Receive and parse freeze frame data */
    if (J2534_ReadMsgs(0, response, &msg_count, 1000) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive freeze frame");
        return -1;
    }
    
    for (size_t i = 0; i < msg_count; i += 4) {
        FreezeFrame* frame = &data[frame_count++];
        frame->dtc = dtc;
        frame->pid = response[i];
        memcpy(frame->data, &response[i + 1], 3);
        
        /* Calculate value based on PID */
        switch (frame->pid) {
            case 0x04:  /* Engine Load */
                frame->value = calculate_engine_load(frame->data[0]);
                break;
            case 0x05:  /* Coolant Temperature */
                frame->value = calculate_coolant_temp(frame->data[0]);
                break;
            case 0x0C:  /* RPM */
                frame->value = calculate_rpm(frame->data[0], frame->data[1]);
                break;
            default:
                frame->value = 0.0f;
                break;
        }
    }
    
    *count = frame_count;
    return 0;
}

/* Clear DTCs */
int diag_clear_dtcs(void) {
    PID_Request req = {OBD_MODE_CLEAR_TROUBLE_CODES, 0x00};
    
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Clearing DTCs");
    
    if (obd2_send_request(&req) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to clear DTCs");
        return -1;
    }
    
    /* Wait for confirmation */
    PID_Response resp;
    if (obd2_receive_response(&resp) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "No confirmation for DTC clear");
        return -1;
    }
    
    return resp.data[0] == 0x44 ? 0 : -1;  /* 0x44 = Clear successful */
}
