#include "obd2_core.h"
#include <stdio.h>
#include <string.h>

/* Command line argument handling */
static int handle_diagnostic_command(const char* command) {
    if (strcmp(command, "--diag-health") == 0) {
        SystemHealth health;
        return diag_get_system_health(&health);
    }
    else if (strcmp(command, "--test-j2534") == 0) {
        return diag_test_j2534_device();
    }
    else if (strcmp(command, "--test-voltage") == 0) {
        return diag_verify_voltage_levels();
    }
    else if (strcmp(command, "--test-signal") == 0) {
        return diag_check_signal_quality();
    }
    else if (strcmp(command, "--test-performance") == 0) {
        return diag_measure_response_times();
    }
    else if (strcmp(command, "--test-throughput") == 0) {
        return diag_test_throughput();
    }
    else if (strcmp(command, "--test-memory") == 0) {
        return diag_memory_test();
    }
    else if (strcmp(command, "--test-buffer") == 0) {
        return diag_test_buffer_operations();
    }
    else if (strcmp(command, "--test-network") == 0) {
        return diag_test_network_connectivity();
    }
    else if (strcmp(command, "--analyze-protocol") == 0) {
        return diag_test_protocol_stack();
    }
    else if (strncmp(command, "--stress-test", 12) == 0) {
        uint32_t duration = 300; // Default 5 minutes
        if (strstr(command, "--duration=") != NULL) {
            sscanf(strstr(command, "--duration=") + 10, "%u", &duration);
        }
        return diag_stress_test(duration);
    }
    
    return -1;
}

int main(int argc, char* argv[]) {
    /* Initialize debug system */
    debug_print_init();
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "OBD2 Diagnostic Tool Starting...");
    
    /* Check for diagnostic commands */
    if (argc > 1) {
        return handle_diagnostic_command(argv[1]);
    }
    
    /* Normal operation */
    HardwareManager hwManager;
    LogBuffer logBuffer;
    PID_Request request;
    PID_Response response;
    LogEntry logEntry;
    float value;

    /* Initialize systems */
    if (obd2_init() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize OBD2");
        return 1;
    }

    if (hw_init(&hwManager) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize hardware manager");
        return 1;
    }

    if (log_init(&logBuffer, 1024) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize log buffer");
        return 1;
    }

    /* Add hardware features */
    hw_add_feature(&hwManager, FEATURE_WIDEBAND_O2);
    hw_add_feature(&hwManager, FEATURE_BOOST_CONTROL);

    /* Example: Read RPM */
    request.mode = 0x01;
    request.pid = 0x0C;
    
    if (obd2_send_request(&request) == 0 && 
        obd2_receive_response(&response) == 0) {
        
        value = calculate_rpm(response.data[0], response.data[1]);
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Current RPM: %.2f", value);

        /* Log the RPM reading */
        logEntry.timestamp = (uint32_t)time(NULL);
        logEntry.pid = request.pid;
        logEntry.dataLength = 2;
        logEntry.data[0] = response.data[0];
        logEntry.data[1] = response.data[1];
        logEntry.processedValue = value;
        logEntry.priority = PRIORITY_HIGH;
        
        log_write(&logBuffer, &logEntry);
    }

    /* Example: Read O2 sensor */
    value = hw_read_value(&hwManager, FEATURE_WIDEBAND_O2);
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "O2 Sensor: %.3f lambda", value);

    /* Cleanup */
    log_free(&logBuffer);

    return 0;
}
