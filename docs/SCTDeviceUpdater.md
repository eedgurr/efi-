# SCT Device Updater
The SCT Device Updater application is a buggy piece of software that handles updating firmware, tunes, strategies, and other functions of SCT devices.

## Initial Startup
The application calls out to this URL to determine if it needs to self update.
http://cdn.derivesystems.com/sctfiles/SoftwareUpdates/SCTDeviceUpdater/currentsctdeviceupdater.txt
Note: All calls to the CDN should use a cache break of some sort at the end of the URL such as "?this=that".

The response will be a pipe and comma delimited list of update data. It contains the version numbers to their corresponding platform builds.
```
Executable32|2.9.16033.08|23797760,Executable64|2.9.16033.08|23797760
```
Where the sections can be expressed as this regular expression: `#Executable32\|([0-9\.]*?)\|([0-9]*?),Executable64\|([0-9\.]*?)\|([0-9]*?)#`

## Driver Update Check
The application will call out to one of these two URLs, depending on the platform, to determine if driver updates are needed.
http://cdn.derivesystems.com/sctfiles/SoftwareUpdates/DriverUpgrade/latestdriver_x64.txt
http://cdn.derivesystems.com/sctfiles/SoftwareUpdates/DriverUpgrade/latestdriver_x86.txt

# SCT Device Integration and Custom Tuning Guide

## Overview
This document details how to maintain compatibility with SCT device functions while adding custom tuning capabilities.

## Original SCT Functions (Preserved)
```cpp
struct SCTParameters {
    uint16_t engineRPM;
    uint16_t vehicleSpeed;
    uint16_t engineLoad;
    uint16_t throttlePosition;
    uint16_t airFuelRatio;
    uint16_t timing;
    uint16_t boostPressure;
} __attribute__((packed));

class SCTDevice {
    // ...existing SCT functions preserved...
};
```

## Custom Tuning Extension
```cpp
class CustomTuningExtension {
private:
    SCTDevice& sctDevice;
    std::map<std::string, TuningProfile> customProfiles;

public:
    struct TuningProfile {
        // Base parameters
        float fuelMultiplier;      // 0.8 to 1.2
        float timingOffset;        // -5 to +5 degrees
        float boostLimit;          // PSI
        float revLimit;            // RPM
        
        // Advanced parameters
        struct {
            float lowRPM;          // Below 2500 RPM
            float midRPM;          // 2500-4500 RPM
            float highRPM;         // Above 4500 RPM
        } fuelAdjustments;
        
        struct {
            float lowBoost;        // 0-5 PSI
            float midBoost;        // 5-10 PSI
            float highBoost;       // 10+ PSI
        } timingAdjustments;
    };

    bool createCustomProfile(const std::string& name, const TuningProfile& profile) {
        if (validateProfile(profile)) {
            customProfiles[name] = profile;
            return true;
        }
        return false;
    }

    bool applyCustomProfile(const std::string& name) {
        auto it = customProfiles.find(name);
        if (it == customProfiles.end()) {
            return false;
        }

        // Preserve SCT base calibration
        SCTParameters baseParams = sctDevice.getCurrentParameters();
        
        // Apply custom adjustments
        applyFuelAdjustments(it->second, baseParams);
        applyTimingAdjustments(it->second, baseParams);
        applyBoostControl(it->second, baseParams);
        
        return sctDevice.updateParameters(baseParams);
    }

private:
    void applyFuelAdjustments(const TuningProfile& profile, SCTParameters& params) {
        // Apply RPM-based fuel adjustments while preserving SCT safety limits
        float rpmBased = getRPMBasedAdjustment(profile.fuelAdjustments, params.engineRPM);
        params.airFuelRatio = adjustWithinLimits(
            params.airFuelRatio,
            rpmBased * profile.fuelMultiplier,
            0.8f,  // Minimum multiplier
            1.2f   // Maximum multiplier
        );
    }

    void applyTimingAdjustments(const TuningProfile& profile, SCTParameters& params) {
        // Apply boost-based timing adjustments while preserving SCT safety limits
        float boostBased = getBoostBasedAdjustment(profile.timingAdjustments, params.boostPressure);
        params.timing = adjustWithinLimits(
            params.timing,
            profile.timingOffset + boostBased,
            -5.0f,  // Maximum retard
            5.0f    // Maximum advance
        );
    }

    bool validateProfile(const TuningProfile& profile) {
        // Validate all parameters are within safe ranges
        return (profile.fuelMultiplier >= 0.8f && profile.fuelMultiplier <= 1.2f) &&
               (profile.timingOffset >= -5.0f && profile.timingOffset <= 5.0f) &&
               (profile.boostLimit >= 0.0f && profile.boostLimit <= 25.0f) &&
               (profile.revLimit >= 2000.0f && profile.revLimit <= 8000.0f);
    }
};
```

## Advanced Tuning Parameters
```cpp
struct AdvancedTuningProfile {
    // Base SCT compatibility
    SCTParameters sctBase;

    // Extended Fuel Management
    struct FuelManagement {
        float volumetricEfficiency[24];  // VE table by RPM
        float injectorScaling;           // Base pulse width multiplier
        struct AFRTargets {
            float idle;          // 14.7:1 typical
            float cruise;        // 14.3:1 typical
            float wot;          // 12.5:1 typical
            float acceleration; // 12.0:1 typical
            float deceleration; // 15.0:1 typical
        } afrTargets;
        
        struct InjectorTiming {
            float startAngle;    // Degrees BTDC
            float endAngle;      // Degrees ATDC
            float deadTime;      // Injector latency compensation
        } injectorTiming;
    } fuelManagement;

    // Extended Timing Management
    struct TimingManagement {
        float baseAdvance[24][16];  // Base timing map (RPM x Load)
        struct DynamicTiming {
            float knockRetard;      // Maximum timing pull
            float coldAdvance;      // Extra advance when cold
            float iactRetard;       // Intake air correction
            float altitudeTrim;     // Altitude compensation
        } dynamicTiming;
        
        struct IdleTiming {
            float targetRPM;        // Desired idle RPM
            float maxAdvance;       // Maximum idle advance
            float minAdvance;       // Minimum idle advance
        } idleTiming;
    } timingManagement;

    // Extended Boost Control
    struct BoostControl {
        float targetBoost[12][12];  // Target boost map (RPM x TPS)
        struct WastegateDuty {
            float minDuty;          // Minimum wastegate duty
            float maxDuty;          // Maximum wastegate duty
            float rampRate;         // PSI per second
        } wastegate;
        
        struct BoostSafety {
            float maxBoost;         // Absolute maximum boost
            float cutThreshold;     // Boost cut threshold
            float resumeThreshold;  // Boost resume threshold
        } safety;
    } boostControl;
};
```

## Real-time Monitoring Interface
```cpp
class TuningMonitor {
private:
    CustomTuningExtension& tuner;
    std::vector<SensorReading> readings;

public:
    struct SensorReading {
        uint32_t timestamp;
        float actualAFR;
        float targetAFR;
        float actualBoost;
        float targetBoost;
        float actualTiming;
        float targetTiming;
    };

    void logReading(const SensorReading& reading) {
        readings.push_back(reading);
        analyzeFeedback(reading);
    }

    void analyzeFeedback(const SensorReading& reading) {
        // Implement adaptive learning based on real-time feedback
        // This helps optimize the custom tune while maintaining safety
    }
};
```

## Enhanced Safety System
```cpp
class EnhancedSafetySystem {
private:
    struct SafetyLimits {
        // Engine Protection
        float maxRPM;              // Maximum engine RPM
        float maxBoost;            // Maximum boost pressure
        float maxEGT;              // Maximum exhaust gas temperature
        float minOilPressure;      // Minimum oil pressure
        float maxCoolantTemp;      // Maximum coolant temperature
        float maxKnockRetard;      // Maximum knock retard
        float minAFR;              // Minimum air/fuel ratio (richest)
        float maxAFR;              // Maximum air/fuel ratio (leanest)
        
        // Transmission Protection
        float maxTorqueConverter;  // Maximum torque converter slip
        float maxTransTemp;        // Maximum transmission temperature
        
        // Driveline Protection
        float maxTorque;           // Maximum torque output
        float maxWheelSlip;        // Maximum wheel slip percentage
    };

    SafetyLimits limits;
    std::vector<EmergencyAction> emergencyActions;

public:
    bool validateParameters(const AdvancedTuningProfile& profile) {
        // Comprehensive parameter validation
        return validateFuelParams(profile.fuelManagement) &&
               validateTimingParams(profile.timingManagement) &&
               validateBoostParams(profile.boostControl);
    }

    void monitorRealTime() {
        while (true) {
            SensorData data = getSensorData();
            
            if (!isWithinSafeLimits(data)) {
                executeEmergencyProcedure(data);
            }
            
            updateSafetyLog(data);
            delay(10);  // 100Hz monitoring
        }
    }

protected:
    void executeEmergencyProcedure(const SensorData& data) {
        // Emergency actions priority
        if (data.boostPressure > limits.maxBoost) {
            cutBoost();
        }
        if (data.engineRPM > limits.maxRPM) {
            cutFuel();
        }
        if (data.knockRetard > limits.maxKnockRetard) {
            pullTiming();
        }
        
        logEmergencyEvent(data);
    }
};
```

## Comprehensive Monitoring System
```cpp
class AdvancedMonitoring {
private:
    struct MonitoringData {
        // Performance Metrics
        struct Performance {
            float horsepower;
            float torque;
            float boost;
            float airflowRate;
            float volumetricEfficiency;
        } performance;

        // Engine Health
        struct EngineHealth {
            float knockCount;
            float misfireCount;
            float compressionBalance[8];
            float cylinderContribution[8];
        } engineHealth;

        // Fuel System
        struct FuelMetrics {
            float shortTermFuel[2];  // Bank 1 & 2
            float longTermFuel[2];   // Bank 1 & 2
            float injectorDutyCycle;
            float fuelRailPressure;
        } fuelMetrics;

        // Environmental
        struct Environmental {
            float barometricPressure;
            float ambientTemp;
            float relativeHumidity;
            float altitude;
        } environmental;
    };

    std::deque<MonitoringData> dataHistory;
    DataLogger logger;

public:
    void startMonitoring() {
        while (true) {
            MonitoringData data = collectData();
            analyzeData(data);
            updateAdaptiveLearning(data);
            logData(data);
            
            if (needsAdjustment(data)) {
                performAutoTune(data);
            }
            
            delay(20);  // 50Hz monitoring
        }
    }
};
```

## Advanced Adaptive Learning System
```cpp
class AdaptiveLearningSystem {
private:
    struct LearningTable {
        float fuelCorrections[16][16];    // RPM x Load
        float timingCorrections[16][16];   // RPM x Load
        float boostCorrections[12][12];    // RPM x TPS
        
        struct AdaptiveState {
            float confidenceLevel;
            uint32_t sampleCount;
            float variance;
            float lastUpdate;
        } state[16][16];
    };

    LearningTable learningData;
    NeuralNetwork aiModel;

public:
    void updateLearning(const MonitoringData& data) {
        // Process new data
        std::vector<float> features = extractFeatures(data);
        std::vector<float> targets = calculateTargets(data);
        
        // Update AI model
        aiModel.train(features, targets);
        
        // Apply learned corrections
        updateFuelCorrections(data);
        updateTimingCorrections(data);
        updateBoostCorrections(data);
        
        // Validate changes
        validateCorrections();
    }

    TuningAdjustments predictOptimalTune(const SensorData& current) {
        return aiModel.predict(current);
    }

protected:
    void updateFuelCorrections(const MonitoringData& data) {
        // Update fuel learning based on AFR feedback
        int rpmIndex = getRPMIndex(data.performance.engineRPM);
        int loadIndex = getLoadIndex(data.performance.engineLoad);
        
        float targetAFR = getTargetAFR(data);
        float actualAFR = data.fuelMetrics.actualAFR;
        float correction = calculateFuelCorrection(targetAFR, actualAFR);
        
        learningData.fuelCorrections[rpmIndex][loadIndex] = 
            smoothCorrection(correction, 
                           learningData.fuelCorrections[rpmIndex][loadIndex],
                           learningData.state[rpmIndex][loadIndex].confidenceLevel);
    }
};
```

## Custom Tuning Interface
```cpp
class TuningInterface {
public:
    void createPerformanceProfile() {
        CustomTuningExtension::TuningProfile profile;
        profile.fuelMultiplier = 1.1f;     // 10% more fuel
        profile.timingOffset = 2.0f;       // 2 degrees advance
        profile.boostLimit = 15.0f;        // 15 PSI max
        profile.revLimit = 7000.0f;        // 7000 RPM limit
        
        // RPM-specific fuel adjustments
        profile.fuelAdjustments.lowRPM = 1.05f;
        profile.fuelAdjustments.midRPM = 1.10f;
        profile.fuelAdjustments.highRPM = 1.15f;
        
        // Boost-specific timing adjustments
        profile.timingAdjustments.lowBoost = 2.0f;
        profile.timingAdjustments.midBoost = 1.0f;
        profile.timingAdjustments.highBoost = 0.0f;
        
        tuner.createCustomProfile("Performance", profile);
    }
};
```

## Enhanced Tuning Interface
```cpp
void configureFuelManagement(AdvancedTuningProfile& profile) {
    // Configure VE table
    for (int rpm = 0; rpm < 24; rpm++) {
        profile.fuelManagement.volumetricEfficiency[rpm] = 
            calculateBaseVE(rpm * 250);  // 250 RPM increments
    }
    
    // Configure AFR targets
    profile.fuelManagement.afrTargets.idle = 14.7f;
    profile.fuelManagement.afrTargets.cruise = 14.3f;
    profile.fuelManagement.afrTargets.wot = 12.5f;
    profile.fuelManagement.afrTargets.acceleration = 12.0f;
    profile.fuelManagement.afrTargets.deceleration = 15.0f;
    
    // Configure injector timing
    profile.fuelManagement.injectorTiming.startAngle = 355.0f;
    profile.fuelManagement.injectorTiming.endAngle = 120.0f;
    profile.fuelManagement.injectorTiming.deadTime = 0.98f;
}
```

## Usage Example
```cpp
// Initialize systems
SCTDevice sctDevice;
CustomTuningExtension tuner(sctDevice);
SafetyMonitor safety;
TuningMonitor monitor(tuner);

// Create and apply custom profile
TuningInterface interface;
interface.createPerformanceProfile();
tuner.applyCustomProfile("Performance");

// Monitor real-time data
while (true) {
    auto reading = getCurrentSensorData();
    monitor.logReading(reading);
    
    // Safety check every cycle
    if (!safety.validateAdjustments(
            sctDevice.getCurrentParameters(),
            tuner.getAdjustedParameters())) {
        // Revert to safe parameters if needed
        tuner.revertToSafeProfile();
    }
    
    delay(100); // Update every 100ms
}
```

## Usage Example with Advanced Features
```cpp
int main() {
    // Initialize systems
    AdvancedTuningProfile profile;
    EnhancedSafetySystem safety;
    AdvancedMonitoring monitor;
    AdaptiveLearningSystem learning;
    
    // Load base SCT calibration
    profile.sctBase = loadSCTCalibration();
    
    // Configure advanced parameters
    configureFuelManagement(profile);
    configureTimingManagement(profile);
    configureBoostControl(profile);
    
    // Start monitoring and safety systems
    std::thread safetyThread(&EnhancedSafetySystem::monitorRealTime, &safety);
    std::thread monitorThread(&AdvancedMonitoring::startMonitoring, &monitor);
    
    // Main control loop
    while (true) {
        // Get current data
        MonitoringData data = monitor.getCurrentData();
        
        // Update adaptive learning
        learning.updateLearning(data);
        
        // Apply optimized tune if conditions are met
        if (safety.validateParameters(profile)) {
            TuningAdjustments adjustments = learning.predictOptimalTune(data);
            applyTuningAdjustments(adjustments);
        }
        
        delay(100);  // Main loop runs at 10Hz
    }
    
    return 0;
}
```

## Best Practices
1. Always maintain SCT's built-in safety limits
2. Implement gradual parameter changes
3. Monitor sensor feedback continuously
4. Keep detailed logs of all modifications
5. Include fallback mechanisms
6. Validate all parameter changes before applying
7. Use adaptive learning to optimize tunes

## Best Practices and Safety Guidelines
1. Always maintain SCT's core safety features
2. Implement progressive parameter changes
3. Use multi-stage validation
4. Keep comprehensive logs
5. Monitor all critical parameters
6. Include multiple fallback mechanisms
7. Validate all changes against known good values
8. Implement gradual learning rates
9. Use confidence-based corrections
10. Maintain separate safety and performance thresholds

## Safety Considerations
1. Never exceed SCT's maximum safe values
2. Implement multiple monitoring systems
3. Include automatic reversion mechanisms
4. Log all parameter changes for analysis
5. Use progressive ramping for parameter changes

# Cross-Platform SCT Device Connectivity Guide

## Universal Connection Manager
```cpp
class SCTConnectionManager {
private:
    enum ConnectionType {
        BLUETOOTH,
        WIFI,
        USB
    };

    struct DeviceInfo {
        std::string deviceId;
        std::string firmwareVersion;
        ConnectionType type;
        bool isCustomFirmware;
        uint32_t capabilities;
    };

public:
    // Platform-agnostic connection interface
    virtual bool connect(const DeviceInfo& device) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() = 0;
    virtual bool sendData(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receiveData() = 0;
};

// Bluetooth Implementation
class BluetoothManager : public SCTConnectionManager {
private:
    // Cross-platform Bluetooth implementation
    #ifdef __ANDROID__
        BluetoothAdapter* btAdapter;
    #elif defined(__APPLE__)
        CBCentralManager* btManager;
    #elif defined(_WIN32)
        HANDLE btHandle;
    #else
        BluetoothAdapter* btAdapter;  // Linux
    #endif

public:
    std::vector<DeviceInfo> scanForDevices() {
        std::vector<DeviceInfo> devices;
        
        // Platform specific scanning
        #ifdef __ANDROID__
            startAndroidScan();
        #elif defined(__APPLE__)
            startIOSScan();
        #elif defined(_WIN32)
            startWindowsScan();
        #else
            startLinuxScan();
        #endif
        
        return devices;
    }

    bool connect(const DeviceInfo& device) override {
        if (device.type != BLUETOOTH) return false;
        
        // Implement platform-specific connection
        bool success = false;
        
        #ifdef __ANDROID__
            success = connectAndroid(device);
        #elif defined(__APPLE__)
            success = connectIOS(device);
        #elif defined(_WIN32)
            success = connectWindows(device);
        #else
            success = connectLinux(device);
        #endif
        
        if (success) {
            initializeCustomProtocol();
        }
        
        return success;
    }
};

// WiFi Implementation
class WiFiManager : public SCTConnectionManager {
private:
    static const uint16_t DEFAULT_PORT = 35000;
    std::unique_ptr<TCPSocket> socket;
    
    struct WiFiConfig {
        std::string ssid;
        std::string password;
        std::string ipAddress;
        uint16_t port;
        bool useEncryption;
    };

public:
    bool connectToDevice(const WiFiConfig& config) {
        socket = std::make_unique<TCPSocket>();
        
        if (!socket->connect(config.ipAddress, config.port)) {
            return false;
        }
        
        if (config.useEncryption) {
            return setupSecureChannel();
        }
        
        return true;
    }

    bool setupAccessPoint(const WiFiConfig& config) {
        // Create WiFi access point for direct connection
        return initializeAP(config);
    }
};
```

## Custom Protocol Implementation
```cpp
class SCTProtocol {
private:
    static const uint8_t PROTOCOL_VERSION = 0x01;
    static const uint16_t MAX_PACKET_SIZE = 1024;
    
    struct PacketHeader {
        uint8_t version;
        uint8_t command;
        uint16_t length;
        uint32_t sequence;
        uint8_t flags;
    } __attribute__((packed));

public:
    enum Command {
        CMD_HANDSHAKE = 0x01,
        CMD_READ_TUNE = 0x02,
        CMD_WRITE_TUNE = 0x03,
        CMD_READ_PARAMS = 0x04,
        CMD_WRITE_PARAMS = 0x05,
        CMD_START_LOGGING = 0x06,
        CMD_STOP_LOGGING = 0x07,
        CMD_FLASH_FIRMWARE = 0x08
    };

    bool sendCommand(Command cmd, const std::vector<uint8_t>& data) {
        PacketHeader header;
        header.version = PROTOCOL_VERSION;
        header.command = static_cast<uint8_t>(cmd);
        header.length = data.size();
        header.sequence = getNextSequence();
        header.flags = calculateFlags();
        
        return sendPacket(header, data);
    }
};
```

## Cross-Platform UI Integration
```cpp
class SCTDeviceUI {
public:
    // Qt/QML interface for desktop
    #ifdef USE_QT
    void setupQtInterface() {
        qmlRegisterType<SCTDeviceModel>("SCT", 1, 0, "SCTDevice");
        qmlRegisterType<ConnectionManager>("SCT", 1, 0, "ConnectionManager");
        engine.load(QUrl("qrc:/qml/Main.qml"));
    }
    #endif

    // Android native interface
    #ifdef __ANDROID__
    void setupAndroidInterface() {
        // Android specific UI setup
    }
    #endif

    // iOS native interface
    #ifdef __APPLE__
    void setupIOSInterface() {
        // iOS specific UI setup
    }
    #endif
};
```

## Custom Firmware Implementation
```cpp
class SCTCustomFirmware {
private:
    struct FirmwareHeader {
        uint32_t magic;
        uint8_t version;
        uint32_t size;
        uint32_t crc32;
        uint8_t compatibility[16];
    } __attribute__((packed));

public:
    bool flashCustomFirmware(const std::vector<uint8_t>& firmware) {
        // Validate firmware
        if (!validateFirmware(firmware)) {
            return false;
        }
        
        // Create backup
        if (!createBackup()) {
            return false;
        }
        
        // Flash firmware
        return programFirmware(firmware);
    }

protected:
    bool validateFirmware(const std::vector<uint8_t>& firmware) {
        FirmwareHeader* header = (FirmwareHeader*)firmware.data();
        
        // Check magic number
        if (header->magic != 0x53435446) { // "SCTF"
            return false;
        }
        
        // Verify CRC
        uint32_t calculated = calculateCRC32(
            firmware.data() + sizeof(FirmwareHeader),
            firmware.size() - sizeof(FirmwareHeader)
        );
        
        return calculated == header->crc32;
    }
};
```

## Usage Example
```cpp
int main() {
    // Initialize connection managers
    BluetoothManager btManager;
    WiFiManager wifiManager;
    SCTProtocol protocol;
    
    // Scan for devices
    auto devices = btManager.scanForDevices();
    
    // Connect to first available device
    if (!devices.empty()) {
        if (btManager.connect(devices[0])) {
            // Upload custom firmware
            SCTCustomFirmware firmware;
            std::vector<uint8_t> firmwareData = loadFirmware("custom.bin");
            
            if (firmware.flashCustomFirmware(firmwareData)) {
                // Start communication with custom protocol
                protocol.sendCommand(SCTProtocol::CMD_HANDSHAKE, {});
                
                // Begin tuning operations
                AdvancedTuningProfile profile;
                configureTuningProfile(profile);
                
                std::vector<uint8_t> tuneData = serializeProfile(profile);
                protocol.sendCommand(SCTProtocol::CMD_WRITE_TUNE, tuneData);
            }
        }
    }
    
    return 0;
}
```

## Best Practices
1. Always validate device compatibility before flashing
2. Implement robust error handling for all connections
3. Use secure communication when possible
4. Maintain connection state monitoring
5. Implement automatic reconnection
6. Handle platform-specific permissions
7. Validate all received data
8. Implement proper cleanup on disconnection

## Security Considerations
1. Encrypt all sensitive communications
2. Validate firmware signatures
3. Implement secure bootloader
4. Use session-based authentication
5. Monitor for unauthorized access attempts
6. Implement rate limiting
7. Log all critical operations

# SCT Device Hardware Enhancement Guide

## Priority-Based Logging System

The enhanced SCT device firmware supports prioritized logging and additional hardware features. This allows you to:
1. Prioritize specific PIDs and sensors
2. Configure high-speed logging for critical parameters
3. Add support for additional hardware sensors

### Configuration Example

```cpp
// Initialize priority-based logging
HardwareFeatureManager featureManager;

// Set priorities for different PIDs
featureManager.setPIDPriority(0x0C, HardwareFeatureManager::CRITICAL);  // RPM
featureManager.setPIDPriority(0x0B, HardwareFeatureManager::HIGH);      // MAP
featureManager.setPIDPriority(0x05, HardwareFeatureManager::MEDIUM);    // Coolant Temp
featureManager.setPIDPriority(0x1F, HardwareFeatureManager::LOW);       // Runtime

// Initialize high-speed logging
HighSpeedLogger logger;

// Configure additional hardware
HardwareManager hwManager;
hwManager.initializeHardware(HardwareFeature::WIDEBAND_O2);
hwManager.initializeHardware(HardwareFeature::BOOST_CONTROL);
```

### Supported Additional Hardware

1. Wideband O2 Sensor
   - Sample Rate: Up to 100Hz
   - Resolution: 0.001 Lambda
   - Range: 0.5 - 2.0 Lambda

2. Boost Control
   - Control Rate: Up to 50Hz
   - Pressure Range: -14.7 to 60 PSI
   - Integrated safety cutoffs

3. Knock Detection
   - Sample Rate: Up to 200kHz
   - Frequency Range: 5kHz - 15kHz
   - Multiple cylinder support

4. MAP Sensor
   - Sample Rate: Up to 1kHz
   - Range: -14.7 to 60 PSI
   - Temperature compensated

5. Flex Fuel Sensor
   - Update Rate: 50Hz
   - Ethanol Content: 0-100%
   - Fuel Temperature

### Data Logging Configuration

```cpp
struct LoggingConfig {
    uint16_t sampleRate;        // Samples per second
    uint8_t priority;           // Priority level 1-5
    bool enableHighSpeed;       // Enable high-speed buffer
    size_t bufferSize;         // Buffer size in entries
    std::string outputFormat;  // "binary" or "csv"
};

// Example configuration for different scenarios
LoggingConfig performanceLogging = {
    .sampleRate = 100,
    .priority = 1,
    .enableHighSpeed = true,
    .bufferSize = 1024,
    .outputFormat = "binary"
};

LoggingConfig diagnosticLogging = {
    .sampleRate = 10,
    .priority = 3,
    .enableHighSpeed = false,
    .bufferSize = 256,
    .outputFormat = "csv"
};
```

### Hardware Integration Example

```cpp
class SCTHardwareIntegration {
public:
    void initialize() {
        // Initialize hardware features
        hwManager.initializeHardware(HardwareFeature::WIDEBAND_O2);
        hwManager.initializeHardware(HardwareFeature::BOOST_CONTROL);
        
        // Set up logging priorities
        for (const auto& pid : criticalPIDs) {
            featureManager.setPIDPriority(pid, HardwareFeatureManager::CRITICAL);
        }
        
        // Configure logging
        logger.setConfig(performanceLogging);
    }
    
    void processData() {
        // Read OBD2 data
        PID_Response response = readOBD2Data();
        
        // Read additional hardware
        float o2 = hwManager.readHardwareValue(HardwareFeature::WIDEBAND_O2);
        float boost = hwManager.readHardwareValue(HardwareFeature::BOOST_CONTROL);
        
        // Log all data
        logger.logPID(response);
        logger.logCustomValue(o2, "WIDEBAND_O2");
        logger.logCustomValue(boost, "BOOST");
    }

private:
    HardwareManager hwManager;
    HardwareFeatureManager featureManager;
    HighSpeedLogger logger;
    std::vector<uint8_t> criticalPIDs = {0x0C, 0x0B, 0x11, 0x0F};
};
```