#include "realtime_monitor.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Monitor state */
static struct {
    MonitorConfig config;
    MonitorSample* history;
    size_t history_size;
    size_t history_head;
    FILE* log_file;
    uint8_t running;
} monitor_state = {0};

/* Initialize monitoring */
int monitor_init(const MonitorConfig* config) {
    if (!config) return -1;
    
    // Copy configuration
    monitor_state.config = *config;
    
    // Allocate history buffer
    monitor_state.history = malloc(sizeof(MonitorSample) * config->buffer_size);
    if (!monitor_state.history) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to allocate monitor history");
        return -1;
    }
    
    // Open log file if needed
    if (config->log_to_file) {
        monitor_state.log_file = fopen(config->log_file, "w");
        if (!monitor_state.log_file) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to open monitor log file");
            free(monitor_state.history);
            return -1;
        }
        
        // Write header
        fprintf(monitor_state.log_file, "Timestamp");
        for (size_t i = 0; i < config->pid_count; i++) {
            fprintf(monitor_state.log_file, ",PID_%02X", config->pids[i]);
        }
        fprintf(monitor_state.log_file, "\n");
    }
    
    return 0;
}

/* Start monitoring */
int monitor_start(void) {
    if (monitor_state.running) return 0;
    
    monitor_state.running = 1;
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Real-time monitoring started");
    
    // Start monitoring thread or timer here
    return 0;
}

/* Stop monitoring */
int monitor_stop(void) {
    if (!monitor_state.running) return 0;
    
    monitor_state.running = 0;
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Real-time monitoring stopped");
    
    // Stop monitoring thread or timer here
    return 0;
}

/* Get latest sample */
int monitor_get_latest(MonitorSample* sample) {
    if (!monitor_state.running || !sample) return -1;
    
    size_t idx = (monitor_state.history_head - 1) % monitor_state.config.buffer_size;
    *sample = monitor_state.history[idx];
    return 0;
}

/* DTC functions */
static const char* dtc_database_path = "docs/DTC_Database.txt";

int monitor_get_dtc_description(const char* code, char* desc, size_t size) {
    FILE* db = fopen(dtc_database_path, "r");
    if (!db) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to open DTC database");
        return -1;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), db)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char* token = strtok(line, "|");
        if (token && strcmp(token, code) == 0) {
            token = strtok(NULL, "|");
            if (token) {
                strncpy(desc, token, size);
                fclose(db);
                return 0;
            }
        }
    }
    
    fclose(db);
    return -1;
}

static void monitor_collect_data(void) {
    if (!monitor_state.running) return;
    
    MonitorSample* sample = &monitor_state.history[monitor_state.history_head];
    sample->timestamp = time(NULL);
    
    // Request data for each configured PID
    for (size_t i = 0; i < monitor_state.config.pid_count; i++) {
        uint8_t data[8];
        size_t length = sizeof(data);
        uint8_t pid = monitor_state.config.pids[i];
        
        if (monitor_state.device->read_pid(0x01, pid, data, &length) == 0) {
            // Process the data based on PID type
            float value = process_pid_data(pid, data, length);
            sample->values[i] = value;
            sample->status[i] = 1;  // Valid data
            
            // Log to file if enabled
            if (monitor_state.log_file) {
                if (i == 0) {
                    fprintf(monitor_state.log_file, "%u", sample->timestamp);
                }
                fprintf(monitor_state.log_file, ",%f", value);
            }
        } else {
            sample->values[i] = 0.0f;
            sample->status[i] = 0;  // Invalid/no data
            if (monitor_state.log_file && i == 0) {
                fprintf(monitor_state.log_file, "%u,ERROR", sample->timestamp);
            }
        }
    }
    
    if (monitor_state.log_file) {
        fprintf(monitor_state.log_file, "\n");
        fflush(monitor_state.log_file);
    }
    
    // Update history head
    monitor_state.history_head = (monitor_state.history_head + 1) % monitor_state.config.buffer_size;
    if (monitor_state.history_size < monitor_state.config.buffer_size) {
        monitor_state.history_size++;
    }
}

static float process_pid_data(uint8_t pid, const uint8_t* data, size_t length) {
    float result = 0.0f;
    
    switch (pid) {
        case 0x04:  // Calculated engine load
            result = (float)data[0] * 100.0f / 255.0f;
            break;
        case 0x05:  // Engine coolant temperature
            result = (float)data[0] - 40.0f;
            break;
        case 0x0C:  // Engine RPM
            result = ((float)(data[0] * 256 + data[1])) / 4.0f;
            break;
        case 0x0D:  // Vehicle speed
            result = (float)data[0];
            break;
        case 0x0F:  // Intake air temperature
            result = (float)data[0] - 40.0f;
            break;
        case 0x11:  // Throttle position
            result = (float)data[0] * 100.0f / 255.0f;
            break;
        default:
            // Return raw first byte for unknown PIDs
            result = (float)data[0];
            break;
    }
    
    return result;
}

int monitor_clear_history(void) {
    if (!monitor_state.history) return -1;
    
    memset(monitor_state.history, 0, 
           sizeof(MonitorSample) * monitor_state.config.buffer_size);
    monitor_state.history_head = 0;
    monitor_state.history_size = 0;
    
    return 0;
}

int monitor_check_dtc(void) {
    uint8_t data[32];
    size_t length = sizeof(data);
    
    // Mode 03: Request trouble codes
    if (monitor_state.device->read_pid(0x03, 0x00, data, &length) != 0) {
        return -1;
    }
    
    // Process and store DTCs
    size_t dtc_count = length / 2;  // Each DTC is 2 bytes
    for (size_t i = 0; i < dtc_count; i++) {
        char dtc[6];
        format_dtc(data + (i * 2), dtc);
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Found DTC: %s", dtc);
    }
    
    return dtc_count;
}

int monitor_clear_dtc(void) {
    uint8_t data = 0;
    size_t length = 1;
    
    // Mode 04: Clear trouble codes
    return monitor_state.device->write_pid(0x04, 0x00, &data, length);
}

static void format_dtc(const uint8_t* data, char* dtc) {
    static const char* prefix[] = {"P", "C", "B", "U"};
    
    uint8_t type = (data[0] >> 6) & 0x03;
    sprintf(dtc, "%s%02X%02X", 
            prefix[type],
            data[0] & 0x3F,
            data[1]);
}
