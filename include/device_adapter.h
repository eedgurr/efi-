#ifndef DEVICE_ADAPTER_H
#define DEVICE_ADAPTER_H

#include "obd2_core.h"

/* Device Types */
typedef enum {
    DEVICE_J2534,
    DEVICE_ELM327,
    DEVICE_ARDUINO,
    DEVICE_ESP32,
    DEVICE_SCT,      // Added SCT device support
    DEVICE_SIMULATOR // Added simulator for demo mode
} DeviceType;

/* Connection Types */
typedef enum {
    CONN_USB,
    CONN_BLUETOOTH,
    CONN_WIFI,
    CONN_SERIAL,
    CONN_CUSTOM,     // For SCT's proprietary connection
    CONN_DEMO        // For demo/simulation mode
} ConnectionType;

/* Video Capture Configuration */
typedef struct {
    bool enabled;
    uint32_t resolution_width;    // Video width in pixels
    uint32_t resolution_height;   // Video height in pixels
    uint32_t frame_rate;         // Frames per second
    bool overlay_telemetry;      // Overlay telemetry data on video
    char codec[32];             // Video codec (e.g., "h264")
    struct {
        bool record_audio;       // Record audio with video
        uint32_t audio_bitrate;  // Audio bitrate in kbps
        char audio_codec[32];    // Audio codec
    } audio_config;
} VideoConfig;

/* Telemetry Configuration */
typedef struct {
    bool enabled;
    uint32_t update_rate;        // Telemetry update rate in Hz
    bool enable_live_streaming;   // Enable live data streaming
    struct {
        char server_url[256];    // Streaming server URL
        uint16_t port;           // Server port
        bool use_ssl;            // Use secure connection
        char api_key[64];        // API key for authentication
    } streaming_config;
    struct {
        bool save_to_file;       // Save telemetry to file
        char output_format[32];  // Output format (csv, json, etc.)
        uint32_t buffer_size;    // Buffer size in KB
    } storage_config;
} TelemetryConfig;

/* Sensor Sample Rates */
typedef struct {
    uint32_t accelerometer_hz;  // Accelerometer sample rate
    uint32_t gps_hz;           // GPS update rate
    uint32_t obd_hz;           // OBD data sample rate
} SampleRates;

/* GPS Configuration */
typedef struct {
    bool enabled;
    uint32_t min_accuracy;     // Minimum accuracy in meters
    uint32_t update_interval;  // Update interval in ms
    bool high_precision_mode;  // Enable high precision mode if available
} GPSConfig;

/* Accelerometer Configuration */
typedef struct {
    bool enabled;
    uint32_t sample_rate;      // Samples per second
    float sensitivity;         // G-force sensitivity
    bool high_g_mode;         // Enable high-G mode for racing
    uint8_t filter_level;     // Noise filter level 0-255
} AccelerometerConfig;

/* Log Format Configuration */
typedef struct {
    bool enable_xdf;          // Enable XDF format logging
    bool enable_a2l;          // Enable A2L format logging
    char xdf_template[256];   // Path to XDF template file
    char a2l_file[256];      // Path to A2L description file
    struct {
        uint32_t buffer_size; // Log buffer size in KB
        bool compression;     // Enable log compression
        char output_dir[256]; // Log output directory
    } log_config;
} LogFormatConfig;

/* Connection Configuration */
typedef struct {
    const char* port;       // Serial port or IP address
    uint32_t baudrate;      // Baud rate for serial connections
    uint16_t timeout_ms;    // Communication timeout
    struct {               // iOS specific configuration
        bool high_performance_mode;
        uint32_t screen_refresh_rate;
        bool enable_metal_renderer;
        uint32_t buffer_size;
    } ios_config;
    
    // Add sensor configurations
    GPSConfig gps_config;
    AccelerometerConfig accel_config;
    LogFormatConfig log_format;
    SampleRates sample_rates;
} ConnectionConfig;

/* Device Configuration */
typedef struct {
    DeviceType type;
    ConnectionType conn_type;
    ConnectionConfig conn_config;
    
    union {
        struct {  // SCT specific
            uint32_t protocol_version;
            bool advanced_features;
            bool high_speed_logging;
            uint32_t max_sample_rate;
            bool safety_features;
        } sct;
        
        struct {  // Performance monitoring
            float engine_displacement;
            uint32_t log_interval_ms;
            bool high_precision_timing;
            uint8_t display_brightness;
            struct {
                uint32_t primary_can_baud;
                uint32_t secondary_can_baud;
                bool multi_bus_enabled;
            } can_config;
        } performance;

        struct {  // Demo/Simulator config
            bool enable_realistic_noise;
            uint32_t update_rate_hz;
            float sensor_lag_ms;
            bool simulate_connection_issues;
            struct {
                uint32_t display_refresh_rate;
                bool enable_animations;
                bool high_performance_mode;
                bool use_metal_renderer;
            } ios_display;
        } demo;
    } device_config;
} DeviceConfig;

/* Device Interface */
typedef struct {
    int (*init)(const DeviceConfig* config);
    int (*connect)(void);
    int (*disconnect)(void);
    int (*send_request)(const PID_Request* req);
    int (*receive_response)(PID_Response* resp);
    int (*set_protocol)(uint8_t protocol);
    int (*get_voltage)(float* voltage);
    int (*get_status)(uint8_t* status);
    
    // Performance monitoring functions
    int (*start_performance_logging)(void);
    int (*stop_performance_logging)(void);
    int (*get_performance_data)(PerformanceData* data);
    int (*configure_can_bus)(uint8_t bus_id, uint32_t baud_rate);
    int (*set_display_brightness)(uint8_t level);

    // iOS specific functions
    int (*configure_metal_renderer)(bool enabled);
    int (*set_screen_refresh_rate)(uint32_t hz);
    int (*handle_background_mode)(bool entering_background);
    int (*optimize_power_consumption)(void);
} DeviceInterface;

/* Function Declarations */
int device_init(const DeviceConfig* config);
DeviceInterface* device_get_interface(DeviceType type);
int device_set_real_time_monitoring(uint8_t enabled);
int device_read_dtc_codes(DTCInfo* dtcs, size_t* count);

#endif /* DEVICE_ADAPTER_H */
