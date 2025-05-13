# Bluetooth Protocol Implementation Guide

## Overview
This document details the implementation of Bluetooth communication in the OBD2 scanner, including both classic Bluetooth and BLE implementations.

## Protocol Stack
1. Physical Layer
   - Bluetooth 4.2 + BLE
   - Frequency hopping spread spectrum (FHSS)
   - 2.4 GHz band operation

2. Data Link Layer
   - L2CAP (Logical Link Control and Adaptation Protocol)
   - Flow control implementation
   - Error detection and correction

## Protocol Stack Implementation
```cpp
// Protocol Constants
#define START_DELIMITER      0x7E
#define END_DELIMITER       0x7F
#define ESCAPE_CHAR         0x7D
#define MAX_PACKET_SIZE     256

class BluetoothProtocol {
private:
    enum State {
        WAITING_START,
        READING_LENGTH,
        READING_COMMAND,
        READING_DATA,
        READING_CHECKSUM
    };
    
    State state;
    uint8_t buffer[MAX_PACKET_SIZE];
    uint16_t bufferIndex;
    uint8_t expectedLength;
    bool escapeNext;

public:
    BluetoothProtocol() : state(WAITING_START), bufferIndex(0), escapeNext(false) {}
    
    void processData(uint8_t byte) {
        switch(state) {
            case WAITING_START:
                if(byte == START_DELIMITER) {
                    state = READING_LENGTH;
                    bufferIndex = 0;
                    escapeNext = false;
                }
                break;
                
            case READING_LENGTH:
                expectedLength = byte;
                state = READING_COMMAND;
                break;
                
            case READING_COMMAND:
                buffer[bufferIndex++] = byte;
                state = READING_DATA;
                break;
                
            case READING_DATA:
                if(byte == ESCAPE_CHAR) {
                    escapeNext = true;
                } else {
                    if(escapeNext) {
                        buffer[bufferIndex++] = byte ^ 0x20;
                        escapeNext = false;
                    } else {
                        buffer[bufferIndex++] = byte;
                    }
                    
                    if(bufferIndex >= expectedLength) {
                        state = READING_CHECKSUM;
                    }
                }
                break;
                
            case READING_CHECKSUM:
                if(validateChecksum(byte)) {
                    processPacket();
                }
                state = WAITING_START;
                break;
        }
    }
};
```

## Packet Structure
```
[Start Delimiter (1 byte)] [Length (1 byte)] [Command (1 byte)] [Data (0-255 bytes)] [Checksum (1 byte)]
```

## Packet Structure Implementation
```cpp
struct BluetoothPacket {
    uint8_t length;
    uint8_t command;
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t checksum;
    
    uint8_t calculateChecksum() {
        uint8_t sum = length + command;
        for(int i = 0; i < length - 1; i++) {
            sum += data[i];
        }
        return ~sum + 1;  // Two's complement
    }
    
    void encode(uint8_t* output, uint16_t* outputSize) {
        uint16_t index = 0;
        
        output[index++] = START_DELIMITER;
        output[index++] = length;
        output[index++] = command;
        
        for(int i = 0; i < length - 1; i++) {
            if(data[i] == START_DELIMITER || data[i] == END_DELIMITER || data[i] == ESCAPE_CHAR) {
                output[index++] = ESCAPE_CHAR;
                output[index++] = data[i] ^ 0x20;
            } else {
                output[index++] = data[i];
            }
        }
        
        checksum = calculateChecksum();
        output[index++] = checksum;
        output[index++] = END_DELIMITER;
        
        *outputSize = index;
    }
};
```

### Command Types
- 0x01: Real-time data request
- 0x02: DTC request
- 0x03: Clear DTC
- 0x04: Sensor calibration
- 0x05: Configuration update

## Error Handling
1. Transmission Errors
   - CRC validation
   - Packet sequence numbering
   - Automatic retransmission

2. Connection Management
   - Keep-alive mechanism
   - Connection timeout handling
   - Auto-reconnect implementation

```cpp
class BluetoothErrorHandler {
public:
    enum ErrorType {
        ERR_NONE = 0,
        ERR_TIMEOUT,
        ERR_CHECKSUM,
        ERR_OVERFLOW,
        ERR_INVALID_COMMAND
    };
    
    struct ErrorInfo {
        ErrorType type;
        uint32_t timestamp;
        uint8_t details[32];
    };
    
private:
    static const uint8_t ERROR_HISTORY_SIZE = 10;
    ErrorInfo errorHistory[ERROR_HISTORY_SIZE];
    uint8_t errorIndex;
    
public:
    void logError(ErrorType type, const uint8_t* details = nullptr) {
        ErrorInfo& error = errorHistory[errorIndex];
        error.type = type;
        error.timestamp = millis();
        
        if(details) {
            memcpy(error.details, details, 32);
        }
        
        errorIndex = (errorIndex + 1) % ERROR_HISTORY_SIZE;
    }
    
    bool getLastError(ErrorInfo* error) {
        if(errorIndex == 0) return false;
        
        uint8_t lastIndex = (errorIndex - 1) % ERROR_HISTORY_SIZE;
        *error = errorHistory[lastIndex];
        return true;
    }
};
```

## Connection Management
```cpp
class BluetoothConnectionManager {
private:
    static const uint32_t KEEP_ALIVE_INTERVAL = 5000;
    static const uint32_t CONNECTION_TIMEOUT = 10000;
    
    uint32_t lastKeepAlive;
    uint32_t lastDataReceived;
    bool isConnected;
    
public:
    BluetoothConnectionManager() : 
        lastKeepAlive(0), 
        lastDataReceived(0), 
        isConnected(false) {}
    
    void update() {
        uint32_t currentTime = millis();
        
        // Send keep-alive if needed
        if(isConnected && (currentTime - lastKeepAlive >= KEEP_ALIVE_INTERVAL)) {
            sendKeepAlive();
            lastKeepAlive = currentTime;
        }
        
        // Check for timeout
        if(isConnected && (currentTime - lastDataReceived >= CONNECTION_TIMEOUT)) {
            handleDisconnect();
        }
    }
    
    void handleDataReceived() {
        lastDataReceived = millis();
        if(!isConnected) {
            handleConnect();
        }
    }
    
private:
    void handleConnect() {
        isConnected = true;
        // Connection established handlers
    }
    
    void handleDisconnect() {
        isConnected = false;
        // Connection lost handlers
    }
    
    void sendKeepAlive() {
        BluetoothPacket packet;
        packet.command = CMD_KEEP_ALIVE;
        packet.length = 1;
        // Send packet
    }
};
```

## Security Implementation
1. Pairing Process
   - Secure Simple Pairing (SSP)
   - PIN code authentication
   - Out-of-band pairing support

2. Data Encryption
   - AES-128 encryption
   - Key exchange protocol
   - Session key management

```cpp
class BluetoothSecurity {
private:
    static const uint8_t KEY_SIZE = 16;
    uint8_t sessionKey[KEY_SIZE];
    uint8_t deviceKey[KEY_SIZE];
    
public:
    bool performPairing() {
        // Generate random challenge
        uint8_t challenge[8];
        generateRandom(challenge, 8);
        
        // Send challenge to device
        sendChallenge(challenge);
        
        // Receive response
        uint8_t response[32];
        if(!receiveResponse(response)) {
            return false;
        }
        
        // Verify response
        if(!validateResponse(challenge, response)) {
            return false;
        }
        
        // Generate session key
        generateSessionKey();
        return true;
    }
    
    void encryptPacket(uint8_t* data, uint16_t length) {
        uint8_t iv[16];
        generateIV(iv);
        
        AES_CBC_encrypt_buffer(data, data, length, sessionKey, iv);
    }
    
    bool decryptPacket(uint8_t* data, uint16_t length) {
        uint8_t iv[16];
        extractIV(data, iv);
        
        return AES_CBC_decrypt_buffer(data, data, length, sessionKey, iv);
    }
    
private:
    void generateSessionKey() {
        // Implement secure session key generation
    }
};
```

## Power Management
1. Sleep Modes
   - Sniff mode for power saving
   - Wake-on-connection
   - Low power advertising

2. Transmission Power Control
   - Dynamic power adjustment
   - RSSI monitoring
   - Range optimization

```cpp
class BluetoothPowerManager {
public:
    enum PowerMode {
        POWER_NORMAL,
        POWER_SNIFF,
        POWER_SLEEP
    };
    
private:
    PowerMode currentMode;
    uint32_t lastActivity;
    
public:
    void updatePowerMode() {
        uint32_t inactiveTime = millis() - lastActivity;
        
        if(inactiveTime > 30000) {
            setPowerMode(POWER_SLEEP);
        } else if(inactiveTime > 5000) {
            setPowerMode(POWER_SNIFF);
        } else {
            setPowerMode(POWER_NORMAL);
        }
    }
    
    void handleActivity() {
        lastActivity = millis();
        if(currentMode != POWER_NORMAL) {
            setPowerMode(POWER_NORMAL);
        }
    }
    
private:
    void setPowerMode(PowerMode mode) {
        if(mode == currentMode) return;
        
        switch(mode) {
            case POWER_NORMAL:
                // Configure for normal operation
                break;
            case POWER_SNIFF:
                // Configure sniff mode
                break;
            case POWER_SLEEP:
                // Configure sleep mode
                break;
        }
        
        currentMode = mode;
    }
};
```

## Performance Metrics
1. Throughput
   - Maximum data rate: 1 Mbps
   - Effective payload rate: ~723.2 kbps
   - Latency: < 100ms

2. Range
   - Open field: up to 100m
   - In-vehicle: 10m typical
   - Signal strength monitoring

## Integration Guidelines
1. Hardware Requirements
   - Bluetooth 4.2 module
   - External antenna recommended
   - UART interface to main MCU

2. Software Stack
   - Interrupt-driven UART handling
   - Circular buffer implementation
   - State machine design

## Testing Procedures
1. Connection Testing
   - Range testing
   - Interference testing
   - Multi-device connectivity

2. Data Integrity
   - CRC verification
   - Packet loss measurement
   - Latency measurement

## Debugging Tools
1. Bluetooth Protocol Analyzer
   - Packet capture
   - Timing analysis
   - Error detection

2. Mobile Test Application
   - Connection testing
   - Data visualization
   - Performance monitoring
