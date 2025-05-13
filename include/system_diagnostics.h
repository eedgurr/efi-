#ifndef SYSTEM_DIAGNOSTICS_H
#define SYSTEM_DIAGNOSTICS_H

#include "obd2_core.h"

/* System Health Status */
typedef struct {
    uint8_t cpu_load;
    uint32_t memory_used;
    uint32_t memory_total;
    uint32_t disk_space_free;
    uint32_t uptime_seconds;
    uint8_t buffer_usage;
    uint8_t connection_status;
    uint8_t protocol_status;
} SystemHealth;

/* Performance Metrics */
typedef struct {
    float avg_response_time;
    uint32_t requests_per_second;
    uint32_t total_requests;
    uint32_t failed_requests;
    uint32_t timeouts;
    uint32_t buffer_overflows;
    uint32_t checksum_errors;
} PerformanceMetrics;

/* Error Statistics */
typedef struct {
    uint32_t protocol_errors;
    uint32_t hardware_errors;
    uint32_t communication_errors;
    uint32_t buffer_errors;
    uint32_t parsing_errors;
    char last_error[256];
    uint32_t error_timestamp;
} ErrorStats;

/* System Diagnostic Functions */
int diag_init_system_monitor(void);
int diag_get_system_health(SystemHealth* health);
int diag_get_performance_metrics(PerformanceMetrics* metrics);
int diag_get_error_stats(ErrorStats* stats);
int diag_run_self_test(void);
int diag_check_hardware_status(void);
int diag_test_protocol_stack(void);
int diag_memory_test(void);
int diag_benchmark_system(void);

/* Protocol Testing */
int diag_test_can_communication(void);
int diag_test_j1850_communication(void);
int diag_test_kwp_communication(void);
int diag_test_iso9141_communication(void);

/* Hardware Testing */
int diag_test_j2534_device(void);
int diag_verify_voltage_levels(void);
int diag_check_signal_quality(void);

/* Performance Testing */
int diag_measure_response_times(void);
int diag_test_throughput(void);
int diag_stress_test(uint32_t duration_seconds);

/* Report Generation */
int diag_generate_system_report(const char* filename);
int diag_export_performance_data(const char* filename);
int diag_save_error_log(const char* filename);

#endif /* SYSTEM_DIAGNOSTICS_H */
