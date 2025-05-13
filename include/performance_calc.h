#ifndef PERFORMANCE_CALC_H
#define PERFORMANCE_CALC_H

#include "obd2_core.h"
#include <stdint.h>
#include <time.h>

#define MAX_DATALOG_OVERLAY 10  // Maximum number of logs to overlay

#define SAFETY_CHECK_INTERVAL_MS 100
#define MAX_SAFE_RPM 8000
#define MAX_SAFE_BOOST 30.0
#define MAX_SAFE_EGT 1600.0
#define MAX_SAFE_COOLANT_TEMP 230.0
#define MIN_SAFE_OIL_PRESSURE 10.0

/* Advanced Performance Metrics */
typedef struct {
    float knock_retard;        // Knock retard angle
    float afr_deviation;       // Deviation from target AFR
    float boost_onset;         // Time to reach target boost
    float torque_per_rpm;      // Torque curve analysis
    float power_band_start;    // RPM at power band start
    float power_band_end;      // RPM at power band end
    float shift_efficiency;    // Shift timing efficiency
    float traction_loss;       // Estimated traction loss
} PerformanceMetrics;

/* Run Analysis Data */
typedef struct {
    float improvement_points;  // Areas of potential improvement
    float consistency_score;   // Run consistency rating
    char* improvement_notes;   // Detailed improvement suggestions
    struct {
        float launch_score;    // Launch technique rating
        float shift_score;     // Shift timing rating
        float traction_score;  // Traction management rating
    } technique_scores;
} RunAnalysis;

/* Safety Monitoring */
typedef struct {
    bool passive_mode_enabled;      // Only monitor, no active commands
    bool safety_checks_enabled;     // Enable safety limit monitoring
    bool data_validation_enabled;   // Validate sensor data ranges
    float rpm_limit;               // Maximum safe RPM
    float boost_limit;             // Maximum safe boost
    float egt_limit;              // Maximum safe exhaust gas temp
    float coolant_temp_limit;     // Maximum safe coolant temp
    float min_oil_pressure;       // Minimum safe oil pressure
    uint32_t command_timeout_ms;  // Timeout for commands
    struct {
        bool block_active_commands;  // Block non-monitoring commands
        bool log_all_commands;      // Log all commands sent
        bool validate_responses;     // Validate all responses
    } safety_config;
} SafetyMonitor;

/* Enhanced Logging Configuration */
typedef struct {
    char csv_directory[256];     // Directory for CSV logs
    char log_prefix[32];         // Prefix for log files
    bool auto_export_csv;        // Auto export to CSV
    bool include_timestamps;     // Include precise timestamps
    bool compress_logs;          // Compress logs on save
    struct {
        uint32_t buffer_size;    // Size of circular buffer
        uint32_t flush_interval; // Interval to flush to disk
        bool high_precision;     // Use high precision timestamps
    } buffer_config;
    struct {
        bool enabled;           // Enable background saving
        uint32_t interval_ms;   // Save interval
        char backup_dir[256];   // Backup directory
    } auto_save;
} LogConfig;

/* Enhanced Performance Data Structure */
typedef struct {
    // Original performance data
    float volumetric_efficiency;  // Current VE in percentage
    float maf_scaled;            // Scaled MAF reading in g/s
    float torque_actual;         // Calculated engine torque in Nm
    float boost_pressure;        // Boost pressure in PSI
    float air_fuel_ratio;        // Current AFR
    float intake_air_temp;       // IAT in Celsius
    float throttle_position;     // TPS percentage
    float engine_rpm;            // Current RPM
    float vehicle_speed;         // Speed in MPH
    float acceleration;          // G-force
    time_t timestamp;            // Timestamp of reading

    // Additional sensors
    float knock_voltage;       // Knock sensor voltage
    float fuel_pressure;       // Fuel pressure in PSI
    float oil_pressure;       // Oil pressure in PSI
    float trans_temp;          // Transmission temperature
    float coolant_temp;        // Engine coolant temperature
    float exhaust_temp;        // Exhaust gas temperature
    float boost_target;        // Target boost pressure
    float boost_actual;        // Actual boost pressure
    float clutch_position;     // Clutch position (if applicable)
    float gear_ratio;          // Current gear ratio
    float wheel_speed[4];      // Individual wheel speeds
    float lateral_g;           // Lateral G-force

    // Performance calculations
    float hp_per_rpm;          // Power per RPM point
    float torque_per_psi;      // Torque per PSI boost
    float power_to_weight;     // Real-time power-to-weight
    float trap_speed_proj;     // Projected trap speed

    // Timing data
    uint64_t timestamp_us;     // Microsecond timestamp
    float interval_ms;         // Actual sampling interval

    // Additional safety monitoring
    struct {
        bool in_safe_range;         // All parameters within safe range
        uint32_t warning_flags;     // Bitfield of active warnings
        char warning_message[256];  // Current warning message
        float safety_margin;        // Safety margin percentage
    } safety_status;
    
    // Enhanced sensor data
    struct {
        float oil_pressure;         // Oil pressure in PSI
        float oil_temp;            // Oil temperature in F
        float trans_temp;          // Transmission temperature in F
        float diff_temp;           // Differential temperature in F
        float egt[8];             // EGT for up to 8 cylinders
        float knock_level[8];      // Knock level per cylinder
        float fuel_pressure;       // Fuel pressure in PSI
        float brake_pressure;      // Brake pressure in PSI
        float tire_pressure[4];    // Tire pressures
        float tire_temp[4];       // Tire temperatures
    } sensor_data;
    
    // Validation flags
    struct {
        bool data_valid;           // Data validation status
        bool sensors_responding;    // All sensors responding
        bool values_in_range;      // Values within expected ranges
        uint32_t error_flags;      // Error status bits
    } validation;
} PerformanceData;

/* Speed Range Timing */
typedef struct {
    float start_speed;           // Starting speed in MPH
    float end_speed;             // End speed in MPH
    float elapsed_time;          // Time taken between speeds
    float start_timestamp;       // When timing started
    bool in_progress;            // Currently timing this range
    bool completed;              // Range has been completed
} SpeedRangeTime;

/* Drag Race Session Data */
typedef struct {
    float reaction_time;         // Reaction time in seconds
    float sixty_foot;            // 60ft time
    float eighth_mile;           // 1/8 mile time
    float eighth_mile_speed;     // Speed at 1/8 mile
    float thousand_foot;         // 1000ft time
    float quarter_mile;          // 1/4 mile time
    float quarter_mile_speed;    // Speed at 1/4 mile
    float peak_power;            // Peak power during run
    float peak_torque;           // Peak torque during run
    float best_ve;               // Best VE achieved
    SpeedRangeTime sixty_to_130;   // 60-130 MPH time
    SpeedRangeTime hundred_to_150;  // 100-150 MPH time
    uint32_t log_interval_ms;    // Logging interval in milliseconds
    uint8_t brightness_level;    // Display brightness 0-100%
} DragSessionData;

/* Log Overlay Configuration */
typedef struct {
    char log_name[32];        // Name/identifier for the log
    uint32_t color_code;      // Color for display
    bool visible;             // Visibility toggle
    float vertical_offset;    // Vertical offset for overlay
    float scale_factor;       // Scale factor for comparison
} LogOverlayConfig;

/* Firmware Security */
typedef struct {
    uint32_t encryption_key[8];    // 256-bit encryption key
    char device_signature[64];     // Device signature
    uint32_t security_level;       // Security level 1-5
    bool require_authentication;    // Require user authentication
    struct {
        bool verify_checksum;      // Verify firmware checksum
        bool backup_original;      // Backup original firmware
        bool staged_update;        // Use staged update process
        bool rollback_support;     // Support firmware rollback
    } flash_config;
} FirmwareSecurity;

/* Simulation Configuration */
typedef struct {
    bool simulation_enabled;           // Enable simulation mode
    char* simulation_file;            // Path to simulation data file
    float simulation_multiplier;      // Time scaling factor
    struct {
        float rpm_variance;           // RPM random variance
        float boost_variance;         // Boost random variance
        float afr_variance;           // AFR random variance
        bool add_sensor_noise;        // Add realistic sensor noise
        float noise_amplitude;        // Amplitude of sensor noise
    } simulation_params;
    struct {
        bool replay_mode;            // Replay existing log
        char* replay_file;          // Log file to replay
        float replay_speed;         // Replay speed multiplier
    } replay_config;
} SimulationConfig;

/* Log Overlay Configuration */
typedef struct {
    bool overlay_enabled;             // Enable log overlay
    uint32_t max_overlays;           // Maximum number of overlays
    float transparency_level;        // Overlay transparency (0-1)
    struct {
        bool auto_align;             // Auto-align data points
        bool normalize_time;         // Normalize time scales
        bool show_differences;       // Show differences between logs
        uint32_t color_scheme;       // Overlay color scheme
    } overlay_config;
    struct {
        bool enable_statistics;      // Enable statistical analysis
        bool show_min_max;          // Show min/max bands
        bool show_average;          // Show average line
        bool show_deviation;        // Show standard deviation
    } analysis_config;
} OverlayConfig;

/* SCT Firmware Flashing */
typedef struct {
    uint32_t protocol_version;       // SCT protocol version
    char model_number[32];          // SCT device model
    uint32_t current_firmware;      // Current firmware version
    struct {
        bool force_recovery;        // Force recovery mode
        bool high_speed_flash;      // High speed flashing
        uint32_t block_size;       // Flash block size
        uint32_t timeout_ms;       // Flash timeout
    } flash_params;
    struct {
        bool verify_blocks;        // Verify each block
        bool safe_mode;           // Safe mode flashing
        bool preserve_settings;   // Preserve user settings
        char backup_path[256];    // Backup file path
    } safety_params;
} SCTFlashConfig;

/* Custom Parameter Configuration */
typedef struct {
    char name[32];                // Parameter name
    char unit[16];               // Unit of measurement
    float min_value;            // Minimum value
    float max_value;            // Maximum value
    uint32_t pid_code;          // OBD-II PID code
    struct {
        float scale_factor;     // Scaling factor
        float offset;           // Offset value
        char formula[256];      // Custom calculation formula
    } conversion;
    struct {
        bool high_precision;    // Enable high precision
        uint32_t sample_rate;   // Custom sample rate
        bool smooth_data;       // Enable data smoothing
        uint32_t average_samples; // Number of samples to average
    } sampling;
} CustomParameter;

/* Analysis Results */
typedef struct {
    PerformanceMetrics metrics;
    RunAnalysis analysis;
    struct {
        float peak_values[MAX_DATALOG_OVERLAY];
        float average_values[MAX_DATALOG_OVERLAY];
        float variance_values[MAX_DATALOG_OVERLAY];
    } comparative_data;
} AnalysisResults;

/* Performance Calculator Interface */
typedef struct {
    // Calculation functions
    float (*calc_volumetric_efficiency)(float maf, float rpm, float map, float iat);
    float (*calc_actual_torque)(float maf, float rpm, float spark_advance);
    float (*scale_maf_reading)(float raw_maf, float iat, float baro);
    
    // Data logging functions
    int (*start_drag_session)(uint32_t log_interval_ms);
    int (*stop_drag_session)(void);
    int (*get_session_data)(DragSessionData* session);
    int (*get_realtime_data)(PerformanceData* data);
    int (*set_display_brightness)(uint8_t level);
    
    // Multi-CAN support
    int (*set_can_protocol)(uint8_t can_bus, uint32_t baud_rate);
    int (*switch_can_bus)(uint8_t can_bus);
} PerformanceInterface;

/* Function Declarations */
PerformanceInterface* performance_get_interface(void);
int performance_init_logging(const char* log_path);
int performance_set_log_interval(uint32_t interval_ms);
int performance_init_safety_monitor(SafetyMonitor* config);
int performance_set_passive_mode(bool enabled);
int performance_configure_logging(const LogConfig* config);
int performance_export_to_csv(const char* session_id, const char* filepath);
int performance_validate_command(const char* command, bool* is_safe);
int performance_check_safety_limits(const PerformanceData* data, char* warning_msg);
int performance_init_simulation(SimulationConfig* config);
int performance_set_overlay_config(OverlayConfig* config);
int performance_flash_sct_firmware(const char* firmware_path, SCTFlashConfig* config);
int performance_add_custom_parameter(CustomParameter* param);
int performance_start_simulation(void);
int performance_stop_simulation(void);
int performance_verify_firmware(const char* firmware_path, char* signature);
int performance_backup_firmware(const char* backup_path);
int performance_restore_firmware(const char* backup_path);

#endif /* PERFORMANCE_CALC_H */
