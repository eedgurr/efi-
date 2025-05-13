# Network Protocol Implementation Guide

## Overview
This document details the network communication protocols and implementations used in the OBD2 diagnostic system.

## Protocol Stack
1. TCP/IP Implementation
   ```cpp
   class NetworkManager {
       // Socket handling
       // Connection management
       // Data transmission
   };
   ```

2. WebSocket Protocol
   - Real-time data streaming
   - Bi-directional communication
   - Connection management

## WebSocket Implementation
```cpp
class WebSocketServer {
private:
    uWS::Hub hub;
    std::map<uint32_t, uWS::WebSocket<uWS::SERVER>*> clients;
    uint32_t nextClientId = 1;

public:
    void initialize(int port) {
        hub.onConnection([this](uWS::WebSocket<uWS::SERVER>* ws, uWS::HttpRequest req) {
            uint32_t clientId = nextClientId++;
            clients[clientId] = ws;
            ws->setUserData(new uint32_t(clientId));
            handleNewConnection(clientId);
        });

        hub.onMessage([this](uWS::WebSocket<uWS::SERVER>* ws, char* message, 
                           size_t length, uWS::OpCode opCode) {
            uint32_t clientId = *(uint32_t*)ws->getUserData();
            handleMessage(clientId, std::string(message, length));
        });

        hub.onDisconnection([this](uWS::WebSocket<uWS::SERVER>* ws, int code, 
                                 char* message, size_t length) {
            uint32_t clientId = *(uint32_t*)ws->getUserData();
            clients.erase(clientId);
            delete (uint32_t*)ws->getUserData();
            handleDisconnection(clientId);
        });

        hub.listen(port);
        hub.run();
    }

    void broadcast(const std::string& message) {
        for(auto& client : clients) {
            client.second->send(message.c_str(), message.length(), uWS::OpCode::TEXT);
        }
    }
};
```

## REST API Implementation
```cpp
class RESTServer {
private:
    httplib::Server server;
    
public:
    void initialize() {
        // Vehicle Data Endpoint
        server.Get("/api/v1/vehicle/data", [](const httplib::Request& req, 
                                            httplib::Response& res) {
            json response = {
                {"timestamp", getCurrentTimestamp()},
                {"speed", getVehicleSpeed()},
                {"rpm", getEngineRPM()},
                {"temp", getEngineTemp()}
            };
            res.set_content(response.dump(), "application/json");
        });

        // DTC Endpoint
        server.Get("/api/v1/dtc", [](const httplib::Request& req, 
                                   httplib::Response& res) {
            std::vector<DTC> dtcs = readDTCs();
            json response = json::array();
            for(const auto& dtc : dtcs) {
                response.push_back({
                    {"code", dtc.code},
                    {"description", dtc.description}
                });
            }
            res.set_content(response.dump(), "application/json");
        });

        // Command Endpoint
        server.Post("/api/v1/command", [](const httplib::Request& req, 
                                        httplib::Response& res) {
            auto json = json::parse(req.body);
            std::string command = json["command"];
            bool success = executeCommand(command);
            
            res.set_content(
                json({{"success", success}}).dump(),
                "application/json"
            );
        });
    }
};
```

## Data Formats
1. JSON Structure
   ```json
   {
       "timestamp": 1683936000,
       "vehicleId": "VIN123456789",
       "readings": {
           "rpm": 2500,
           "speed": 60,
           "temperature": 90
       }
   }
   ```

2. Binary Protocol
   - Message framing
   - Compression
   - Error detection

## Data Streaming Protocol
```cpp
class DataStreamer {
private:
    static const uint16_t BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    uint16_t bufferIndex = 0;

public:
    struct DataPacket {
        uint32_t timestamp;
        struct {
            float rpm;
            float speed;
            float temp;
            float throttle;
        } sensors;
        uint8_t checksum;
    } __attribute__((packed));

    void streamData(const DataPacket& packet) {
        // Add header
        buffer[bufferIndex++] = 0xFF;
        buffer[bufferIndex++] = 0xFE;
        
        // Add timestamp
        memcpy(&buffer[bufferIndex], &packet.timestamp, sizeof(uint32_t));
        bufferIndex += sizeof(uint32_t);
        
        // Add sensor data
        memcpy(&buffer[bufferIndex], &packet.sensors, sizeof(packet.sensors));
        bufferIndex += sizeof(packet.sensors);
        
        // Calculate and add checksum
        uint8_t checksum = calculateChecksum(buffer, bufferIndex);
        buffer[bufferIndex++] = checksum;
        
        // Send data
        sendBuffer(buffer, bufferIndex);
        bufferIndex = 0;
    }
};
```

## Security Layer
1. SSL/TLS Implementation
   - Certificate management
   - Key exchange
   - Cipher suites

2. Authentication
   - Token-based auth
   - OAuth 2.0
   - JWT implementation

## Protocol Security
```cpp
class SecureProtocol {
private:
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
    
public:
    bool initialize() {
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_x509_crt_init(&cert);
        mbedtls_pk_init(&pkey);
        
        // Load certificates
        int ret = mbedtls_x509_crt_parse_file(&cert, "server.crt");
        if(ret != 0) return false;
        
        ret = mbedtls_pk_parse_keyfile(&pkey, "server.key", "");
        if(ret != 0) return false;
        
        // Configure SSL
        ret = mbedtls_ssl_config_defaults(&conf,
                                        MBEDTLS_SSL_IS_SERVER,
                                        MBEDTLS_SSL_TRANSPORT_STREAM,
                                        MBEDTLS_SSL_PRESET_DEFAULT);
        if(ret != 0) return false;
        
        mbedtls_ssl_conf_ca_chain(&conf, &cert, NULL);
        ret = mbedtls_ssl_conf_own_cert(&conf, &cert, &pkey);
        if(ret != 0) return false;
        
        ret = mbedtls_ssl_setup(&ssl, &conf);
        return ret == 0;
    }
};
```

## API Endpoints
1. Real-time Data
   ```
   POST /api/v1/readings
   GET /api/v1/vehicle/{id}/status
   WS /ws/v1/live-data
   ```

2. Historical Data
   - Data retrieval
   - Aggregation
   - Filtering

## Connection Management
```cpp
class ConnectionManager {
private:
    struct Client {
        std::string id;
        uint32_t lastHeartbeat;
        uint32_t messageCount;
        bool authenticated;
    };
    
    std::map<std::string, Client> clients;
    static const uint32_t TIMEOUT_MS = 30000;

public:
    void update() {
        uint32_t currentTime = getCurrentTime();
        auto it = clients.begin();
        
        while(it != clients.end()) {
            if(currentTime - it->second.lastHeartbeat > TIMEOUT_MS) {
                handleTimeout(it->first);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }

    void handleHeartbeat(const std::string& clientId) {
        if(clients.find(clientId) != clients.end()) {
            clients[clientId].lastHeartbeat = getCurrentTime();
        }
    }
};
```

## Error Handling
```cpp
class NetworkErrorHandler {
public:
    enum class ErrorCode {
        NONE,
        CONNECTION_LOST,
        TIMEOUT,
        INVALID_DATA,
        AUTHENTICATION_FAILED
    };
    
    struct ErrorEvent {
        ErrorCode code;
        std::string description;
        uint32_t timestamp;
        std::string clientId;
    };
    
private:
    std::queue<ErrorEvent> errorQueue;
    std::function<void(const ErrorEvent&)> errorCallback;
    
public:
    void setErrorCallback(std::function<void(const ErrorEvent&)> callback) {
        errorCallback = callback;
    }
    
    void handleError(ErrorCode code, const std::string& description, 
                    const std::string& clientId) {
        ErrorEvent event = {
            code,
            description,
            getCurrentTime(),
            clientId
        };
        
        errorQueue.push(event);
        if(errorCallback) {
            errorCallback(event);
        }
    }
};
```

## Performance Optimization
1. Connection Management
   - Connection pooling
   - Keep-alive
   - Load balancing

2. Data Optimization
   - Compression
   - Batching
   - Caching

## Data Compression
```cpp
class DataCompressor {
private:
    z_stream strm;
    static const size_t CHUNK = 16384;
    
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> compressed;
        uint8_t out[CHUNK];
        
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        
        if(deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) {
            return compressed;
        }
        
        strm.avail_in = data.size();
        strm.next_in = (Bytef*)data.data();
        
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            
            deflate(&strm, Z_FINISH);
            
            compressed.insert(compressed.end(), out, 
                            out + CHUNK - strm.avail_out);
        } while(strm.avail_out == 0);
        
        deflateEnd(&strm);
        return compressed;
    }
};
```

## Quality of Service
1. Reliability
   - Acknowledgment system
   - Message ordering
   - Duplicate detection

2. Monitoring
   - Latency tracking
   - Error rates
   - Bandwidth usage

## Implementation Examples
1. Client Side
   ```cpp
   class WebSocketClient {
       void connect(const std::string& url);
       void send(const std::vector<uint8_t>& data);
       void onMessage(std::function<void(const std::vector<uint8_t>&)> callback);
   };
   ```

2. Server Side
   ```python
   class DataServer:
       def handle_connection(self, client):
           # Authentication
           # Session management
           # Data handling
   ```

## Testing Procedures
1. Connection Testing
   - Latency measurement
   - Throughput testing
   - Stability verification

2. Security Testing
   - Penetration testing
   - Vulnerability scanning
   - Encryption validation

## Deployment Considerations
1. Infrastructure
   - Load balancers
   - Proxy servers
   - Firewalls

2. Monitoring
   - Health checks
   - Performance metrics
   - Error logging
