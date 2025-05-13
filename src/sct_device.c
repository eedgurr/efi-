#include "sct_device.h"
#include "device_adapter.h"
#include <string.h>

static SCTDeviceInterface sct_interface;
static DeviceConfig* current_config;

/* Initialize SCT device interface */
int sct_init_device(const DeviceConfig* config) {
    current_config = (DeviceConfig*)config;
    
    /* Initialize SCT protocol */
    if (sct_test_communication() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize SCT device");
        return -1;
    }
    
    /* Check device compatibility */
    if (sct_check_compatibility() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT device compatibility check failed");
        return -1;
    }
    
    /* Setup monitoring system if enabled */
    if (config->device_config.sct.high_speed_logging) {
        if (sct_interface.start_monitoring(config->device_config.sct.max_sample_rate) != 0) {
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to start SCT monitoring");
            return -1;
        }
    }
    
    /* Initialize safety systems if enabled */
    if (config->device_config.sct.safety_features) {
        size_t limits_size;
        uint8_t default_limits[256];
        
        if (sct_interface.get_safety_limits(default_limits, &limits_size) == 0) {
            if (sct_interface.set_safety_limits(default_limits, limits_size) != 0) {
                DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize SCT safety systems");
                return -1;
            }
        }
    }
    
    return 0;
}

/* Test SCT device communication */
int sct_test_communication(void) {
    SCTParameters params;
    
    /* Try to read basic parameters */
    if (sct_interface.get_parameters(&params) != 0) {
        return -1;
    }
    
    return 0;
}

/* Check SCT device compatibility */
int sct_check_compatibility(void) {
    char version[32];
    size_t version_size = sizeof(version);
    
    /* Check firmware version */
    if (sct_interface.check_firmware_version(version, version_size) != 0) {
        return -1;
    }
    
    /* Parse version and check compatibility */
    unsigned int major, minor, patch;
    if (sscanf(version, "%u.%u.%u", &major, &minor, &patch) != 3) {
        return -1;
    }
    
    /* Check minimum required version */
    if (major < 2 || (major == 2 && minor < 9)) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "SCT firmware version too old");
        return -1;
    }
    
    return 0;
}

/* Test SCT device tuning */
int sct_verify_tuning(void) {
    SCTAdvancedTuning tuning;
    
    /* Read current tuning */
    if (sct_interface.get_advanced_tuning(&tuning) != 0) {
        return -1;
    }
    
    /* Verify tuning parameters */
    if (verify_fuel_parameters(&tuning.fuelManagement) != 0 ||
        verify_boost_parameters(&tuning.boostControl) != 0) {
        return -1;
    }
    
    return 0;
}

/* Check logging system status */
int sct_check_logging_status(void) {
    void* buffer;
    size_t size;
    
    /* Try to get monitoring data */
    if (sct_interface.get_monitoring_data(&buffer, &size) != 0) {
        return -1;
    }
    
    return 0;
}

/* Internal verification functions */
static int verify_fuel_parameters(const struct FuelManagement* fuel) {
    /* Check VE table values */
    for (int i = 0; i < 24; i++) {
        if (fuel->volumetricEfficiency[i] < 0.0f || 
            fuel->volumetricEfficiency[i] > 2.0f) {
            return -1;
        }
    }
    
    /* Check AFR targets */
    if (fuel->afrTargets.idle < 10.0f || fuel->afrTargets.idle > 20.0f ||
        fuel->afrTargets.wot < 10.0f || fuel->afrTargets.wot > 15.0f) {
        return -1;
    }
    
    return 0;
}

static int verify_boost_parameters(const struct BoostControl* boost) {
    /* Check boost limits */
    if (boost->maxBoost > 60.0f || boost->targetBoost > boost->maxBoost) {
        return -1;
    }
    
    /* Check safety thresholds */
    if (boost->safety.cutThreshold <= boost->safety.resumeThreshold) {
        return -1;
    }
    
    return 0;
}
