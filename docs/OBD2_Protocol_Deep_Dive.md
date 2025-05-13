# OBD2 Protocol and ECU Communication

## Overview
OBD-II (On-Board Diagnostics II) is a standardized system that monitors vehicle performance and emissions control systems. This document explains the technical aspects of how the system communicates and processes data.

## Protocol Details
1. Communication Protocols
   - ISO 9141-2
   - ISO 14230-4 (KWP2000)
   - ISO 15765-4 (CAN)
   - SAE J1850 PWM
   - SAE J1850 VPW

### Implementation Examples

```cpp
// CAN Protocol Implementation
void initializeCAN() {
    CAN_HandleTypeDef hcan;
    hcan.Instance = CAN1;
    hcan.Init.Prescaler = 16;
    hcan.Init.Mode = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1 = CAN_BS1_4TQ;
    hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
    HAL_CAN_Init(&hcan);
}

// ISO 9141-2 K-Line Implementation
void initializeKLine() {
    UART_HandleTypeDef huart;
    huart.Instance = USART2;
    huart.Init.BaudRate = 10400;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    HAL_UART_Init(&huart);
}
```

## Debug Interface

```c
// Debug configuration
#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_TRACE   5

// Configure debug level
#ifndef OBD_DEBUG_LEVEL
#define OBD_DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// Debug macros
#define DEBUG_PRINT(level, fmt, ...) \
    if (level <= OBD_DEBUG_LEVEL) { \
        debug_print(__FILE__, __LINE__, level, fmt, ##__VA_ARGS__); \
    }

// Debug function prototype
void debug_print(const char* file, int line, int level, const char* fmt, ...);

// Debug implementation
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void debug_print(const char* file, int line, int level, const char* fmt, ...) {
    static const char* level_strings[] = {
        "NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
    };
    
    time_t now;
    time(&now);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0'; // Remove newline
    
    va_list args;
    va_start(args, fmt);
    
    printf("[%s][%s][%s:%d] ", date, level_strings[level], file, line);
    vprintf(fmt, args);
    printf("\n");
    
    va_end(args);
}

// Common data structures for both C and C++
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

// Hardware feature types
typedef enum {
    FEATURE_WIDEBAND_O2 = 0x01,
    FEATURE_BOOST_CONTROL = 0x02,
    FEATURE_KNOCK_SENSOR = 0x03,
    FEATURE_MAP_SENSOR = 0x04,
    FEATURE_FLEX_FUEL = 0x05
} HardwareFeatureType;

typedef struct {
    HardwareFeatureType type;
    uint8_t status;
    uint16_t sampleRate;
    float lastValue;
    uint8_t enabled;
} HardwareFeature;

// Priority levels
typedef enum {
    PRIORITY_CRITICAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_MEDIUM = 3,
    PRIORITY_LOW = 4,
    PRIORITY_LOGGING = 5
} Priority;

// Function prototypes
int initializeOBD(void);
int sendRequest(const PID_Request* req);
int receiveResponse(PID_Response* resp);
float processResponse(const PID_Response* resp);
void logData(const LogEntry* entry);
int initializeHardware(HardwareFeatureType type);
float readHardwareValue(HardwareFeatureType type);

// Example usage with debug
int getRPM(float* rpm) {
    PID_Request req = {0x01, 0x0C};
    PID_Response resp = {0};
    
    DEBUG_PRINT(DEBUG_LEVEL_DEBUG, "Requesting RPM (PID: 0x0C)");
    
    if (sendRequest(&req) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to send RPM request");
        return -1;
    }
    
    if (receiveResponse(&resp) != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to receive RPM response");
        return -1;
    }
    
    *rpm = ((resp.data[0] * 256.0f) + resp.data[1]) / 4.0f;
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "RPM: %.2f", *rpm);
    
    return 0;
}

// Hardware feature implementation with debug
int initializeHardware(HardwareFeatureType type) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing hardware feature: 0x%02X", type);
    
    switch(type) {
        case FEATURE_WIDEBAND_O2:
            // Initialize wideband O2 sensor
            DEBUG_PRINT(DEBUG_LEVEL_DEBUG, "Configuring wideband O2 sensor");
            break;
            
        case FEATURE_BOOST_CONTROL:
            // Initialize boost control
            DEBUG_PRINT(DEBUG_LEVEL_DEBUG, "Configuring boost control");
            break;
            
        default:
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Unknown hardware feature: 0x%02X", type);
            return -1;
    }
    
    return 0;
}

float readHardwareValue(HardwareFeatureType type) {
    float value = 0.0f;
    
    DEBUG_PRINT(DEBUG_LEVEL_DEBUG, "Reading hardware value for feature: 0x%02X", type);
    
    switch(type) {
        case FEATURE_WIDEBAND_O2:
            // Read wideband O2 value
            value = readWidebandO2();
            DEBUG_PRINT(DEBUG_LEVEL_INFO, "Wideband O2: %.3f lambda", value);
            break;
            
        case FEATURE_BOOST_CONTROL:
            // Read boost pressure
            value = readBoostPressure();
            DEBUG_PRINT(DEBUG_LEVEL_INFO, "Boost pressure: %.2f PSI", value);
            break;
            
        default:
            DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Unknown hardware feature: 0x%02X", type);
            return 0.0f;
    }
    
    return value;
}

#ifdef __cplusplus
}  // extern "C"

// C++ wrapper class (optional)
class OBD2Interface {
public:
    OBD2Interface() {
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing OBD2 interface");
        initializeOBD();
    }
    
    float getRPM() {
        float rpm;
        if (::getRPM(&rpm) == 0) {
            return rpm;
        }
        return 0.0f;
    }
    
    void addHardwareFeature(HardwareFeatureType type) {
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Adding hardware feature");
        initializeHardware(type);
    }
    
    float getHardwareValue(HardwareFeatureType type) {
        return readHardwareValue(type);
    }
};
#endif
```

## Hex Commands and PIDs
### Mode 01 PIDs (Real-time Data)
- `0x04`: Engine Load
- `0x05`: Engine Coolant Temperature
- `0x0C`: Engine RPM
- `0x0D`: Vehicle Speed
- `0x0F`: Intake Air Temperature
- `0x11`: Throttle Position

### Implementation Example
```cpp
struct PID_Request {
    uint8_t mode;
    uint8_t pid;
} __attribute__((packed));

struct PID_Response {
    uint8_t mode;
    uint8_t pid;
    uint8_t data[4];
    uint8_t checksum;
} __attribute__((packed));

// Enhanced Logging Structure
struct LogEntry {
    uint32_t timestamp;     // Unix timestamp
    uint16_t pid;          // Parameter ID
    uint8_t dataLength;    // Length of data
    uint8_t data[8];      // Raw data buffer
    float processedValue;  // Converted/calculated value
    uint8_t priority;     // Logging priority (1-5)
} __attribute__((packed));

// Hardware Feature Priority Manager
class HardwareFeatureManager {
public:
    enum Priority {
        CRITICAL = 1,   // Real-time critical data (e.g., engine protection)
        HIGH = 2,       // Important monitoring (e.g., boost control)
        MEDIUM = 3,     // Regular updates (e.g., temperature monitoring)
        LOW = 4,        // Background tasks (e.g., long term fuel trims)
        LOGGING = 5     // Pure data logging
    };

    void setPIDPriority(uint8_t pid, Priority priority) {
        pidPriorities[pid] = priority;
    }

    bool shouldProcess(uint8_t pid) {
        return pidPriorities[pid] <= currentLoadPriority;
    }

private:
    std::map<uint8_t, Priority> pidPriorities;
    Priority currentLoadPriority = CRITICAL;
};

// High-Speed Logging Implementation
class HighSpeedLogger {
public:
    void logPID(const PID_Response& response) {
        LogEntry entry;
        entry.timestamp = getCurrentTime();
        entry.pid = response.pid;
        entry.dataLength = sizeof(response.data);
        memcpy(entry.data, response.data, entry.dataLength);
        entry.processedValue = processRawData(response);
        entry.priority = getPIDPriority(response.pid);
        
        writeToBuffer(entry);
    }

private:
    static constexpr size_t BUFFER_SIZE = 1024;
    LogEntry logBuffer[BUFFER_SIZE];
    size_t bufferIndex = 0;

    void writeToBuffer(const LogEntry& entry) {
        logBuffer[bufferIndex] = entry;
        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
        
        if (bufferIndex == 0) {
            flushBuffer();  // Buffer full, write to storage
        }
    }
};

float getRPM() {
    PID_Request req = {0x01, 0x0C};
    PID_Response resp;
    
    sendOBDRequest(&req);
    if(receiveOBDResponse(&resp)) {
        return ((resp.data[0] * 256) + resp.data[1]) / 4.0;
    }
    return 0.0;
}
```

### Mode 03 (Diagnostic Trouble Codes)
DTC Format: `P0xxx` where:
- P = Powertrain
- 0 = Standard code
- xxx = Specific fault code

### DTC Implementation
```cpp
struct DTC {
    char code[5];
    char description[100];
};

void readDTCs() {
    uint8_t response[256];
    sendOBDRequest(0x03, NULL, 0);
    int length = receiveOBDResponse(response);
    
    for(int i = 0; i < length; i += 2) {
        uint16_t dtc = (response[i] << 8) | response[i+1];
        decodeDTC(dtc);
    }
}
```

## Data Science Applications
1. Real-time Data Analysis
   ```python
   def calculate_rolling_average(data, window_size):
       return np.convolve(data, np.ones(window_size)/window_size, mode='valid')
       
   def detect_anomalies(data, threshold=2):
       mean = np.mean(data)
       std = np.std(data)
       z_scores = [(y - mean) / std for y in data]
       return [abs(z) > threshold for z in z_scores]
   ```

2. Signal Processing
   ```python
   def kalman_filter(data, Q=1e-5, R=1e-2):
       n = len(data)
       x_hat = np.zeros(n)  # Estimate
       P = np.zeros(n)      # Error covariance
       
       # Initialize
       x_hat[0] = data[0]
       P[0] = 1
       
       # Process data
       for k in range(1, n):
           # Prediction
           x_hat_minus = x_hat[k-1]
           P_minus = P[k-1] + Q
           
           # Update
           K = P_minus / (P_minus + R)  # Kalman gain
           x_hat[k] = x_hat_minus + K * (data[k] - x_hat_minus)
           P[k] = (1 - K) * P_minus
           
       return x_hat
   ```

## Mathematical Models
1. Engine Performance
   ```cpp
   class EnginePerformance {
   public:
       float calculateVolumetricEfficiency(float actualMass, float idealMass) {
           return actualMass / idealMass;
       }
       
       float calculateAFR(float airMass, float fuelMass) {
           return airMass / fuelMass;
       }
       
       float calculatePower(float torque, float angularVelocity) {
           return torque * angularVelocity;
       }
   };
   ```

2. Sensor Calibration
   ```cpp
   class SensorCalibration {
   public:
       float calculateTemperature(float resistance) {
           float A = 1.429e-3;
           float B = 2.372e-4;
           float C = 1.019e-7;
           return 1.0 / (A + B * log(resistance) + C * pow(log(resistance), 3));
       }
       
       float calculateMAPPressure(float voltage) {
           float Vmax = 5.0;
           float Pmax = 100.0; // kPa
           return voltage * (Pmax / Vmax);
       }
       
       float calculateO2Lambda(float voltage) {
           // Non-linear response curve
           return voltage < 0.45 ? 1.0 + (0.45 - voltage) / 0.45 :
                                 1.0 - (voltage - 0.45) / 0.45;
       }
   };
   ```

## Additional Hardware Support
```cpp
struct HardwareFeature {
    enum Type {
        WIDEBAND_O2 = 0x01,
        BOOST_CONTROL = 0x02,
        KNOCK_SENSOR = 0x03,
        MAP_SENSOR = 0x04,
        FLEX_FUEL = 0x05
    };

    uint8_t type;
    uint8_t status;
    uint16_t sampleRate;
    float lastValue;
    bool enabled;
};

// Example implementation for handling additional hardware
class HardwareManager {
public:
    void initializeHardware(HardwareFeature::Type type) {
        HardwareFeature feature;
        feature.type = type;
        feature.enabled = true;
        feature.sampleRate = getDefaultSampleRate(type);
        features[type] = feature;
    }

    float readHardwareValue(HardwareFeature::Type type) {
        if (features.count(type) == 0 || !features[type].enabled) {
            return 0.0f;
        }
        
        switch(type) {
            case HardwareFeature::WIDEBAND_O2:
                return readWidebandO2();
            case HardwareFeature::BOOST_CONTROL:
                return readBoostPressure();
            // Add other hardware implementations
            default:
                return 0.0f;
        }
    }

private:
    std::map<HardwareFeature::Type, HardwareFeature> features;
};
```

## Usage Examples

### C Example
```c
int main() {
    float rpm;
    
    // Initialize OBD
    if (initializeOBD() != 0) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to initialize OBD");
        return 1;
    }
    
    // Read RPM
    if (getRPM(&rpm) == 0) {
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Current RPM: %.2f", rpm);
    }
    
    // Initialize and read wideband O2
    if (initializeHardware(FEATURE_WIDEBAND_O2) == 0) {
        float lambda = readHardwareValue(FEATURE_WIDEBAND_O2);
        DEBUG_PRINT(DEBUG_LEVEL_INFO, "Lambda: %.3f", lambda);
    }
    
    return 0;
}
```

### C++ Example
```cpp
int main() {
    OBD2Interface obd;
    
    // Read RPM
    float rpm = obd.getRPM();
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Current RPM: %.2f", rpm);
    
    // Add and read wideband O2
    obd.addHardwareFeature(FEATURE_WIDEBAND_O2);
    float lambda = obd.getHardwareValue(FEATURE_WIDEBAND_O2);
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Lambda: %.3f", lambda);
    
    return 0;
}
```
