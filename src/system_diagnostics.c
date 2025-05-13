#include "system_diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static SystemHealth current_health = {0};
static PerformanceMetrics current_metrics = {0};
static ErrorStats error_stats = {0};

int diag_init_system_monitor(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing system diagnostics");
    memset(&current_health, 0, sizeof(SystemHealth));
    memset(&current_metrics, 0, sizeof(PerformanceMetrics));
    memset(&error_stats, 0, sizeof(ErrorStats));
    return 0;
}

int diag_get_system_health(SystemHealth* health) {
    if (!health) return -1;
    
    /* Get real system metrics */
    FILE* fp;
    char buffer[1024];
    
    /* CPU Load */
    fp = popen("top -bn1 | grep 'Cpu(s)' | awk '{print $2}'", "r");
    if (fp) {
        fgets(buffer, sizeof(buffer), fp);
        current_health.cpu_load = (uint8_t)atof(buffer);
        pclose(fp);
    }
    
    /* Memory Usage */
    fp = popen("free -m | grep 'Mem:' | awk '{print $3,$2}'", "r");
    if (fp) {
        fgets(buffer, sizeof(buffer), fp);
        sscanf(buffer, "%u %u", &current_health.memory_used, 
               &current_health.memory_total);
        pclose(fp);
    }
    
    /* Disk Space */
    fp = popen("df -k . | tail -1 | awk '{print $4}'", "r");
    if (fp) {
        fgets(buffer, sizeof(buffer), fp);
        current_health.disk_space_free = atoi(buffer);
        pclose(fp);
    }
    
    /* Connection and Protocol Status */
    current_health.connection_status = can_check_bus_status() == 0 ? 1 : 0;
    current_health.protocol_status = obd2_protocol_init() == 0 ? 1 : 0;
    
    *health = current_health;
    return 0;
}

int diag_run_self_test(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Running system self-test");
    
    int status = 0;
    
    /* Test J2534 Device */
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing J2534 device...");
    if (diag_test_j2534_device() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "J2534 device test failed");
        status = -1;
    }
    
    /* Test Protocol Stack */
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing protocol stack...");
    if (diag_test_protocol_stack() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Protocol stack test failed");
        status = -1;
    }
    
    /* Device-specific tests */
    switch (current_health.device_type) {
        case DEVICE_ELM327:
            status |= diag_test_elm327_device();
            break;
        case DEVICE_ARDUINO:
            status |= diag_test_arduino_device();
            break;
        case DEVICE_ESP32:
            status |= diag_test_esp32_device();
            break;
        case DEVICE_SCT:
            status |= diag_test_sct_device();
            break;
    }
    
    /* Memory Test */
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing memory...");
    if (diag_memory_test() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Memory test failed");
        status = -1;
    }
    
    return status;
}

int diag_test_protocol_stack(void) {
    int status = 0;
    
    /* Test CAN Protocol */
    if (diag_test_can_communication() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "CAN protocol test failed");
        status = -1;
    }
    
    /* Test J1850 Protocol */
    if (diag_test_j1850_communication() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "J1850 protocol test failed");
        status = -1;
    }
    
    /* Test KWP2000 Protocol */
    if (diag_test_kwp_communication() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "KWP2000 protocol test failed");
        status = -1;
    }
    
    return status;
}

int diag_test_can_communication(void) {
    /* Initialize CAN */
    if (can_init(500000, 0) != 0) {
        return -1;
    }
    
    /* Send test message */
    CANFrame frame = {
        .id = 0x7DF,
        .dlc = 8,
        .data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .is_extended = 0,
        .is_remote = 0
    };
    
    if (can_send_frame(&frame) != 0) {
        return -1;
    }
    
    /* Receive response */
    CANFrame response;
    return can_receive_frame(&response, 1000);
}

/* ELM327 specific tests */
static int diag_test_elm327_device(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing ELM327 device functionality");
    
    int status = 0;
    char response[256];
    DeviceInterface* interface = device_get_interface(DEVICE_ELM327);
    
    /* Test device presence */
    if (interface->send_command("ATZ\r", response, sizeof(response)) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ELM327 reset command failed");
        return -1;
    }
    
    /* Test protocol support */
    if (interface->send_command("AT SP 0\r", response, sizeof(response)) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ELM327 protocol setting failed");
        status = -1;
    }
    
    /* Test voltage reading */
    if (interface->send_command("AT RV\r", response, sizeof(response)) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ELM327 voltage reading failed");
        status = -1;
    }
    
    return status;
}

/* Arduino specific tests */
static int diag_test_arduino_device(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing Arduino device functionality");
    
    int status = 0;
    DeviceInterface* interface = device_get_interface(DEVICE_ARDUINO);
    
    /* Test CAN controller */
    uint8_t data[8] = {0};
    size_t length = sizeof(data);
    
    if (interface->read_pid(0x01, 0x00, data, &length) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Arduino CAN communication failed");
        status = -1;
    }
    
    /* Test SD card if available */
    if (interface->send_command("TEST_SD\r", NULL, 0) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_WARN, "Arduino SD card test failed");
    }
    
    return status;
}

/* ESP32 specific tests */
static int diag_test_esp32_device(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing ESP32 device functionality");
    
    int status = 0;
    DeviceInterface* interface = device_get_interface(DEVICE_ESP32);
    
    /* Test WiFi if enabled */
    if (current_health.conn_type == CONN_WIFI) {
        if (interface->send_command("TEST_WIFI\r", NULL, 0) != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ESP32 WiFi test failed");
            status = -1;
        }
    }
    
    /* Test Bluetooth if enabled */
    if (current_health.conn_type == CONN_BLUETOOTH) {
        if (interface->send_command("TEST_BT\r", NULL, 0) != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ESP32 Bluetooth test failed");
            status = -1;
        }
    }
    
    /* Test CAN controller */
    uint8_t data[8] = {0};
    size_t length = sizeof(data);
    if (interface->read_pid(0x01, 0x00, data, &length) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "ESP32 CAN communication failed");
        status = -1;
    }
    
    return status;
}

/* SCT Device Tests */
static int diag_test_sct_device(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Testing SCT device functionality");
    
    int status = 0;
    SCTDeviceInterface* sct = get_sct_interface();
    
    /* Test device presence and communication */
    if (sct_test_communication() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT communication test failed");
        status = -1;
    }
    
    /* Test firmware version */
    char version[32];
    size_t version_size = sizeof(version);
    if (sct->check_firmware_version(version, version_size) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT firmware version check failed");
        status = -1;
    } else {
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "SCT Firmware Version: %s", version);
    }
    
    /* Test basic parameters */
    SCTParameters params;
    if (sct->get_parameters(&params) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT parameter read failed");
        status = -1;
    }
    
    /* Test advanced tuning if enabled */
    if (current_health.device_config.sct.advanced_features) {
        SCTAdvancedTuning tuning;
        if (sct->get_advanced_tuning(&tuning) != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT advanced tuning read failed");
            status = -1;
        }
    }
    
    /* Test monitoring system */
    if (current_health.device_config.sct.high_speed_logging) {
        if (sct_check_logging_status() != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT monitoring system test failed");
            status = -1;
        }
    }
    
    /* Test safety systems if enabled */
    if (current_health.device_config.sct.safety_features) {
        if (sct->check_safety_status() != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT safety system test failed");
            status = -1;
        }
    }
    
    return status;
}

int diag_generate_system_report(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    
    time_t now = time(NULL);
    SystemHealth health;
    PerformanceMetrics metrics;
    ErrorStats stats;
    
    fprintf(fp, "System Diagnostic Report\n");
    fprintf(fp, "======================\n");
    fprintf(fp, "Generated: %s\n", ctime(&now));
    
    /* System Health */
    diag_get_system_health(&health);
    fprintf(fp, "\nSystem Health:\n");
    fprintf(fp, "CPU Load: %d%%\n", health.cpu_load);
    fprintf(fp, "Memory Used: %d MB / %d MB\n", 
            health.memory_used, health.memory_total);
    fprintf(fp, "Free Disk Space: %d KB\n", health.disk_space_free);
    fprintf(fp, "Connection Status: %s\n", 
            health.connection_status ? "Connected" : "Disconnected");
    
    /* Performance Metrics */
    diag_get_performance_metrics(&metrics);
    fprintf(fp, "\nPerformance Metrics:\n");
    fprintf(fp, "Average Response Time: %.2f ms\n", metrics.avg_response_time);
    fprintf(fp, "Requests/Second: %d\n", metrics.requests_per_second);
    fprintf(fp, "Total Requests: %d\n", metrics.total_requests);
    fprintf(fp, "Failed Requests: %d\n", metrics.failed_requests);
    
    /* Error Statistics */
    diag_get_error_stats(&stats);
    fprintf(fp, "\nError Statistics:\n");
    fprintf(fp, "Protocol Errors: %d\n", stats.protocol_errors);
    fprintf(fp, "Hardware Errors: %d\n", stats.hardware_errors);
    fprintf(fp, "Communication Errors: %d\n", stats.communication_errors);
    fprintf(fp, "Last Error: %s\n", stats.last_error);
    
    fclose(fp);
    return 0;
}
