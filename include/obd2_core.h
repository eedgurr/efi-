#ifndef OBD2_CORE_H
#define OBD2_CORE_H

#include <stdint.h>
#include <time.h>

/* Debug configuration */
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_TRACE   5

#ifndef OBD_DEBUG_LEVEL
#define OBD_DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

/* Data Structures */
typedef struct {
    uint8_t mode;
    uint8_t pid;
} PID_Request;

typedef struct {
    uint8_t mode;
    uint8_t pid;
    uint8_t data[4];
    uint8_t checksum;
} PID_Response;

typedef struct {
    uint32_t timestamp;
    uint16_t pid;
    uint8_t dataLength;
    uint8_t data[8];
    float processedValue;
    uint8_t priority;
} LogEntry;

/* Hardware Types */
typedef enum {
    FEATURE_WIDEBAND_O2 = 0x01,
    FEATURE_BOOST_CONTROL = 0x02,
    FEATURE_KNOCK_SENSOR = 0x03,
    FEATURE_MAP_SENSOR = 0x04,
    FLEX_FUEL = 0x05
} HardwareFeatureType;

typedef struct {
    HardwareFeatureType type;
    uint8_t status;
    uint16_t sampleRate;
    float lastValue;
    uint8_t enabled;
} HardwareFeature;

/* Priority Levels */
typedef enum {
    PRIORITY_CRITICAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_MEDIUM = 3,
    PRIORITY_LOW = 4,
    PRIORITY_LOGGING = 5
} Priority;

/* Logging System */
typedef struct {
    LogEntry* entries;
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
} LogBuffer;

/* Hardware Manager */
typedef struct {
    HardwareFeature features[16];  /* Support up to 16 features */
    size_t featureCount;
} HardwareManager;

/* Function Declarations */
void debug_print(const char* file, int line, int level, const char* fmt, ...);
int obd2_init(void);
int obd2_send_request(const PID_Request* req);
int obd2_receive_response(PID_Response* resp);
float obd2_process_response(const PID_Response* resp);

/* Hardware Functions */
int hw_init(HardwareManager* manager);
int hw_add_feature(HardwareManager* manager, HardwareFeatureType type);
float hw_read_value(HardwareManager* manager, HardwareFeatureType type);

/* Logging Functions */
int log_init(LogBuffer* buffer, size_t capacity);
void log_write(LogBuffer* buffer, const LogEntry* entry);
int log_read(LogBuffer* buffer, LogEntry* entry);
void log_free(LogBuffer* buffer);

/* Utility Functions */
float calculate_engine_load(uint8_t raw_value);
float calculate_coolant_temp(uint8_t raw_value);
float calculate_rpm(uint8_t msb, uint8_t lsb);
float calculate_speed(uint8_t raw_value);
float calculate_timing_advance(uint8_t raw_value);
float calculate_intake_temp(uint8_t raw_value);
float calculate_maf(uint8_t msb, uint8_t lsb);
float calculate_throttle_pos(uint8_t raw_value);
float calculate_o2_voltage(uint8_t raw_value);
float calculate_fuel_level(uint8_t raw_value);

/* Protocol Functions */
int obd2_protocol_init(void);
int obd2_protocol_set_baudrate(uint32_t baudrate);
int obd2_protocol_set_protocol(uint8_t protocol);

/* J1850 Protocol Functions */
int j1850_init(uint8_t protocol);
int j1850_send_message(const uint8_t* data, size_t length);
int j1850_receive_message(uint8_t* data, size_t* length);

/* KWP2000 Protocol Functions */
int kwp_init(void);
int kwp_send_request(uint8_t service_id, const uint8_t* data, size_t length);
int kwp_receive_response(uint8_t* data, size_t* length);

/* CAN Protocol Functions */
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t is_extended;
    uint8_t is_remote;
} CANFrame;

int can_init(uint32_t baudrate, uint8_t extended_id);
int can_send_frame(const CANFrame* frame);
int can_receive_frame(CANFrame* frame, uint32_t timeout_ms);
int can_set_filter(uint32_t id, uint32_t mask, uint8_t extended);
int can_check_bus_status(void);
int can_iso_tp_send(uint32_t id, const uint8_t* data, size_t length);

/* Advanced Diagnostic Functions */
typedef struct {
    char code[5];
    uint16_t raw_code;
    char description[100];
    uint8_t status;
    uint32_t timestamp;
} DTCInfo;

typedef struct {
    uint16_t dtc;
    uint8_t pid;
    uint8_t data[4];
    float value;
} FreezeFrame;

int diag_read_dtcs(DTCInfo* dtcs, size_t* count);
int diag_read_freeze_frame(uint16_t dtc, FreezeFrame* data, size_t* count);
int diag_clear_dtcs(void);

/* Protocol Types */
#define PROTOCOL_AUTO          0
#define PROTOCOL_ISO_9141_2    3
#define PROTOCOL_ISO_14230_4   4
#define PROTOCOL_ISO_15765_4   6
#define PROTOCOL_SAE_J1850_PWM 1
#define PROTOCOL_SAE_J1850_VPW 2

/* Protocol States */
#define PROTOCOL_STATE_UNINITIALIZED  0
#define PROTOCOL_STATE_INITIALIZING   1
#define PROTOCOL_STATE_INITIALIZED    2
#define PROTOCOL_STATE_ERROR         -1

/* OBD Modes */
#define OBD_MODE_SHOW_CURRENT_DATA    0x01
#define OBD_MODE_SHOW_FREEZE_FRAME    0x02
#define OBD_MODE_READ_TROUBLE_CODES   0x03
#define OBD_MODE_CLEAR_TROUBLE_CODES  0x04
#define OBD_MODE_TEST_RESULTS         0x05
#define OBD_MODE_CONTROL_ONBOARD      0x08
#define OBD_MODE_REQUEST_INFO         0x09

/* Debug Macro */
#define DEBUG_PRINT(level, fmt, ...) \
    if (level <= OBD_DEBUG_LEVEL) { \
        debug_print(__FILE__, __LINE__, level, fmt, ##__VA_ARGS__); \
    }

#endif /* OBD2_CORE_H */
