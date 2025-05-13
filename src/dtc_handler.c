#include "dtc_handler.h"
#include "device_adapter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* DTC database structure */
typedef struct {
    char code[6];
    char description[256];
    uint8_t severity;
    char system[64];
} DTCInfo;

static DTCInfo* dtc_database = NULL;
static size_t dtc_count = 0;

int dtc_init_database(const char* database_path) {
    FILE* fp = fopen(database_path, "r");
    if (!fp) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to open DTC database: %s", database_path);
        return -1;
    }
    
    // Count entries
    char line[512];
    size_t count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '#' && line[0] != '\n') {
            count++;
        }
    }
    
    // Allocate memory
    dtc_database = malloc(sizeof(DTCInfo) * count);
    if (!dtc_database) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to allocate DTC database");
        fclose(fp);
        return -1;
    }
    
    // Read entries
    rewind(fp);
    dtc_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char* token = strtok(line, "|");
        if (!token) continue;
        
        // Code
        strncpy(dtc_database[dtc_count].code, token, sizeof(dtc_database[dtc_count].code) - 1);
        
        // Description
        token = strtok(NULL, "|");
        if (token) {
            strncpy(dtc_database[dtc_count].description, token, 
                    sizeof(dtc_database[dtc_count].description) - 1);
        }
        
        // Severity
        token = strtok(NULL, "|");
        if (token) {
            dtc_database[dtc_count].severity = atoi(token);
        }
        
        // System
        token = strtok(NULL, "|");
        if (token) {
            strncpy(dtc_database[dtc_count].system, token, 
                    sizeof(dtc_database[dtc_count].system) - 1);
        }
        
        dtc_count++;
    }
    
    fclose(fp);
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Loaded %zu DTC entries", dtc_count);
    return 0;
}

int dtc_get_info(const char* code, DTCDetails* details) {
    if (!dtc_database || !code || !details) return -1;
    
    for (size_t i = 0; i < dtc_count; i++) {
        if (strcmp(dtc_database[i].code, code) == 0) {
            strncpy(details->description, dtc_database[i].description,
                    sizeof(details->description) - 1);
            details->severity = dtc_database[i].severity;
            strncpy(details->system, dtc_database[i].system,
                    sizeof(details->system) - 1);
            return 0;
        }
    }
    
    return -1;
}

int dtc_read_current(DTCData* data) {
    if (!data) return -1;
    
    uint8_t buffer[128];
    size_t length = sizeof(buffer);
    
    // Mode 03: Get current DTCs
    DeviceInterface* device = device_get_interface(current_device_type);
    if (!device) return -1;
    
    if (device->read_pid(0x03, 0x00, buffer, &length) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to read DTCs");
        return -1;
    }
    
    // Parse DTCs
    data->count = 0;
    for (size_t i = 0; i < length && data->count < MAX_DTC_COUNT; i += 2) {
        DTCEntry* entry = &data->entries[data->count];
        
        // Format DTC code
        uint8_t type = (buffer[i] >> 6) & 0x03;
        sprintf(entry->code, "%c%02X%02X",
                "PCBU"[type],
                buffer[i] & 0x3F,
                buffer[i + 1]);
        
        // Get details from database
        if (dtc_get_info(entry->code, &entry->details) != 0) {
            strncpy(entry->details.description, "Unknown DTC",
                    sizeof(entry->details.description) - 1);
            entry->details.severity = 3;
            strncpy(entry->details.system, "Unknown",
                    sizeof(entry->details.system) - 1);
        }
        
        data->count++;
    }
    
    return 0;
}

int dtc_read_freeze_frame(uint8_t frame_id, DTCData* data) {
    if (!data) return -1;
    
    uint8_t buffer[128];
    size_t length = sizeof(buffer);
    
    // Mode 02: Get freeze frame data
    DeviceInterface* device = device_get_interface(current_device_type);
    if (!device) return -1;
    
    if (device->read_pid(0x02, frame_id, buffer, &length) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to read freeze frame");
        return -1;
    }
    
    // Parse freeze frame data
    parse_freeze_frame_data(buffer, length, data);
    return 0;
}

int dtc_clear_all(void) {
    // Mode 04: Clear DTCs
    DeviceInterface* device = device_get_interface(current_device_type);
    if (!device) return -1;
    
    uint8_t data = 0;
    size_t length = 1;
    
    if (device->write_pid(0x04, 0x00, &data, length) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to clear DTCs");
        return -1;
    }
    
    return 0;
}
