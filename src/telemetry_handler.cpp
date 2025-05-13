#include "../include/device_adapter.h"
#include "../include/performance_calc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_TELEMETRY_BUFFER 1024
#define TELEMETRY_VERSION "1.0"

typedef struct {
    uint64_t timestamp;
    float lat;
    float lon;
    float speed;
    float rpm;
    float boost;
    float throttle;
    float brake;
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    float g_force;
    float slip_angle;
    int gear;
    float track_position;
    float lap_time;
    float sector_time;
    float predicted_lap_time;
} TelemetryFrame;

static TelemetryConfig config;
static FILE* telemetry_file = NULL;
static char buffer[MAX_TELEMETRY_BUFFER];

int telemetry_init(const TelemetryConfig* cfg) {
    memcpy(&config, cfg, sizeof(TelemetryConfig));
    
    if (config.storage_config.save_to_file) {
        char filename[512];
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        
        snprintf(filename, sizeof(filename), 
                "%s/telemetry_%04d%02d%02d_%02d%02d%02d.%s",
                config.storage_config.output_dir,
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec,
                config.storage_config.output_format);
                
        telemetry_file = fopen(filename, "w");
        if (!telemetry_file) return -1;
        
        // Write header
        fprintf(telemetry_file, 
                "timestamp,lat,lon,speed,rpm,boost,throttle,brake,"
                "accel_x,accel_y,accel_z,g_force,slip_angle,gear,"
                "track_pos,lap_time,sector_time,predicted_time\n");
    }
    
    return 0;
}

int telemetry_update(const PerformanceData* data) {
    TelemetryFrame frame = {
        .timestamp = data->timestamp_us,
        .lat = data->sensor_data.gps_lat,
        .lon = data->sensor_data.gps_lon,
        .speed = data->vehicle_speed,
        .rpm = data->engine_rpm,
        .boost = data->boost_actual,
        .throttle = data->throttle_position,
        .brake = data->brake_position,
        .acceleration_x = data->sensor_data.accel_x,
        .acceleration_y = data->sensor_data.accel_y,
        .acceleration_z = data->sensor_data.accel_z,
        .g_force = data->acceleration,
        .slip_angle = data->sensor_data.slip_angle,
        .gear = data->current_gear,
        .track_position = data->sensor_data.track_position,
        .lap_time = data->sensor_data.lap_time,
        .sector_time = data->sensor_data.sector_time,
        .predicted_lap_time = data->sensor_data.predicted_lap_time
    };
    
    if (config.storage_config.save_to_file && telemetry_file) {
        fprintf(telemetry_file, 
                "%llu,%.6f,%.6f,%.2f,%.0f,%.2f,%.2f,%.2f,"
                "%.3f,%.3f,%.3f,%.2f,%.2f,%d,"
                "%.2f,%.3f,%.3f,%.3f\n",
                frame.timestamp, frame.lat, frame.lon,
                frame.speed, frame.rpm, frame.boost,
                frame.throttle, frame.brake,
                frame.acceleration_x, frame.acceleration_y,
                frame.acceleration_z, frame.g_force,
                frame.slip_angle, frame.gear,
                frame.track_position, frame.lap_time,
                frame.sector_time, frame.predicted_lap_time);
    }
    
    if (config.enable_live_streaming) {
        // Implement live streaming logic here
        // For example, using WebSocket or UDP for real-time data
    }
    
    return 0;
}

void telemetry_close(void) {
    if (telemetry_file) {
        fclose(telemetry_file);
        telemetry_file = NULL;
    }
}
