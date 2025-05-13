#ifndef SCT_DEVICE_H
#define SCT_DEVICE_H

#include "device_adapter.h"
#include <stdint.h>

/* SCT device parameters */
typedef struct {
    uint16_t engineRPM;
    uint16_t vehicleSpeed;
    uint16_t engineLoad;
    uint16_t throttlePosition;
    uint16_t airFuelRatio;
    uint16_t timing;
    uint16_t boostPressure;
    uint16_t knockRetard;
    uint16_t fuelPressure;
} SCTParameters;

/* Advanced tuning parameters */
typedef struct {
    struct {
        float volumetricEfficiency[24];  // VE table by RPM
        float injectorScaling;           // Base pulse width multiplier
        struct {
            float idle;          // 14.7:1 typical
            float cruise;        // 14.3:1 typical
            float wot;          // 12.5:1 typical
            float acceleration; // 12.0:1 typical
            float deceleration; // 15.0:1 typical
        } afrTargets;
        
        struct {
            float startAngle;    // Degrees BTDC
            float endAngle;      // Degrees ATDC
            float deadTime;      // Injector latency compensation
        } injectorTiming;
    } fuelManagement;

    struct {
        float maxBoost;         // Maximum boost pressure
        float targetBoost;      // Target boost pressure
        float boostSolenoidDuty;// Boost control duty cycle
        float wastegatePosition;// Wastegate position
        struct {
            float cutThreshold;  // Boost cut threshold
            float resumeThreshold; // Boost resume threshold
        } safety;
    } boostControl;
} SCTAdvancedTuning;

/* SCT device interface extension */
typedef struct {
    /* Basic SCT functions */
    int (*init)(const DeviceConfig* config);
    int (*get_parameters)(SCTParameters* params);
    int (*set_parameters)(const SCTParameters* params);
    
    /* Advanced tuning */
    int (*get_advanced_tuning)(SCTAdvancedTuning* tuning);
    int (*set_advanced_tuning)(const SCTAdvancedTuning* tuning);
    int (*save_tuning_profile)(const char* name);
    int (*load_tuning_profile)(const char* name);
    
    /* Real-time monitoring */
    int (*start_monitoring)(uint16_t sample_rate);
    int (*stop_monitoring)(void);
    int (*get_monitoring_data)(void* data, size_t* size);
    
    /* Safety systems */
    int (*get_safety_limits)(void* limits, size_t* size);
    int (*set_safety_limits)(const void* limits, size_t size);
    int (*check_safety_status)(void);
    int (*handle_safety_event)(uint32_t event_id);
    
    /* Firmware management */
    int (*check_firmware_version)(char* version, size_t size);
    int (*update_firmware)(const uint8_t* data, size_t size);
    int (*verify_firmware)(void);
} SCTDeviceInterface;

/* SCT device diagnostic functions */
int sct_run_diagnostics(void);
int sct_check_compatibility(void);
int sct_test_communication(void);
int sct_verify_tuning(void);
int sct_check_logging_status(void);

#endif /* SCT_DEVICE_H */
