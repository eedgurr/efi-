#ifndef REALTIME_MONITOR_H
#define REALTIME_MONITOR_H

#include "obd2_core.h"
#include "device_adapter.h"

/* Monitoring parameters */
typedef struct {
    uint32_t sample_rate_ms;    // How often to sample
    uint32_t buffer_size;       // How many samples to keep
    uint8_t pids[32];          // PIDs to monitor
    size_t pid_count;          // Number of PIDs
    uint8_t log_to_file;       // Whether to log to file
    char log_file[256];        // Log file path
} MonitorConfig;

/* Sample data */
typedef struct {
    uint32_t timestamp;
    float values[32];          // Values for each PID
    uint8_t status[32];        // Status for each value
} MonitorSample;

/* Monitor interface */
int monitor_init(const MonitorConfig* config);
int monitor_start(void);
int monitor_stop(void);
int monitor_get_latest(MonitorSample* sample);
int monitor_get_history(MonitorSample* samples, size_t* count);
int monitor_clear_history(void);

/* DTC monitoring */
int monitor_check_dtc(void);
int monitor_get_dtc_description(const char* code, char* desc, size_t size);
int monitor_clear_dtc(void);

#endif /* REALTIME_MONITOR_H */
