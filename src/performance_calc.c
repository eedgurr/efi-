#include "performance_calc.h"
#include "device_adapter.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define MAX_LOG_ENTRIES 10000
#define AIR_R_CONSTANT 287.058  // Gas constant for air J/(kg·K)
#define DISPLACEMENT_LITERS 5.0  // Default engine displacement - configurable

static PerformanceData performance_log[MAX_LOG_ENTRIES];
static size_t log_entry_count = 0;
static uint32_t current_log_interval = 100; // Default 100ms
static FILE* log_file = NULL;
static uint8_t current_can_bus = 0;
static float engine_displacement = DISPLACEMENT_LITERS;

// VE calculation using speed-density method
static float calculate_ve(float maf, float rpm, float map, float iat) {
    float air_density = (map * 1000) / (AIR_R_CONSTANT * (iat + 273.15));
    float theoretical_airflow = (engine_displacement * rpm * air_density) / 120.0;
    float actual_airflow = maf;
    return (actual_airflow / theoretical_airflow) * 100.0;
}

// MAF scaling with temperature compensation
static float scale_maf(float raw_maf, float iat, float baro) {
    float temp_factor = sqrt((iat + 273.15) / 298.15);
    float pressure_factor = baro / 101.325;
    return raw_maf * temp_factor * pressure_factor;
}

// Torque calculation using MAF method
static float calculate_torque(float maf, float rpm, float spark_advance) {
    float air_fuel_ratio = 14.7; // Assuming stoichiometric
    float thermal_efficiency = 0.35; // Typical efficiency
    float torque_factor = 120.0 * thermal_efficiency / (2 * M_PI);
    
    // Account for spark timing effect
    float timing_factor = 1.0 + (spark_advance - 20.0) * 0.003;
    
    return (maf * air_fuel_ratio * torque_factor * timing_factor) / rpm;
}

static int start_logging_session(uint32_t interval_ms) {
    log_entry_count = 0;
    current_log_interval = interval_ms;
    
    if (log_file) {
        fprintf(log_file, "Timestamp,RPM,Speed,VE,MAF,Torque,Boost,AFR,IAT,TPS,G-Force\n");
    }
    
    return 0;
}

static int log_performance_data(const PerformanceData* data) {
    if (log_entry_count < MAX_LOG_ENTRIES) {
        performance_log[log_entry_count++] = *data;
        
        if (log_file) {
            fprintf(log_file, "%ld,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f\n",
                    data->timestamp,
                    data->engine_rpm,
                    data->vehicle_speed,
                    data->volumetric_efficiency,
                    data->maf_scaled,
                    data->torque_actual,
                    data->boost_pressure,
                    data->air_fuel_ratio,
                    data->intake_air_temp,
                    data->throttle_position,
                    data->acceleration);
        }
        return 0;
    }
    return -1;
}

static PerformanceInterface perf_interface = {
    .calc_volumetric_efficiency = calculate_ve,
    .calc_actual_torque = calculate_torque,
    .scale_maf_reading = scale_maf,
    .start_drag_session = start_logging_session,
    .stop_drag_session = NULL, // Implemented separately
    .get_session_data = NULL,  // Implemented separately
    .get_realtime_data = NULL, // Implemented separately
    .set_display_brightness = NULL, // Implemented separately
    .set_can_protocol = NULL,  // Implemented separately
    .switch_can_bus = NULL     // Implemented separately
};

PerformanceInterface* performance_get_interface(void) {
    return &perf_interface;
}

int performance_init_logging(const char* log_path) {
    if (log_path) {
        log_file = fopen(log_path, "w");
        return log_file ? 0 : -1;
    }
    return -1;
}

int performance_set_log_interval(uint32_t interval_ms) {
    if (interval_ms >= 10 && interval_ms <= 1000) {
        current_log_interval = interval_ms;
        return 0;
    }
    return -1;
}
