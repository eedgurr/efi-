# Demo Guide

## Overview

The EFI Diagnostic Tool demo provides a fully functional simulation environment for testing and learning. It includes realistic vehicle data simulation, sensor emulation, and full feature access.

## Demo Versions

### 1. Web Demo
- Instant access through browser
- Basic feature demonstration
- No installation required
- [Try Web Demo](https://demo.example.com)

### 2. Desktop Demo
- Full feature set
- Realistic vehicle simulation
- Hardware simulation
- Performance testing

### 3. Mobile Demo
- iOS and Android versions
- Real-time monitoring
- GPS integration
- Video capture

## Quick Start

### Running the Demo

```bash
# Basic demo
./obd2tool --demo

# Demo with specific vehicle
./obd2tool --demo --vehicle="2014 Mustang GT"

# Demo with custom configuration
./obd2tool --demo --config=custom_config.json
```

### Demo Features

#### Basic Features
- Real-time sensor data
- Engine health monitoring
- Performance metrics
- Data logging

#### Advanced Features
- Video capture simulation
- GPS tracking simulation
- Telemetry overlay
- Data export

## Demo Scenarios

### 1. Basic Monitoring
```bash
# Start basic monitoring demo
./obd2tool --demo --scenario=basic_monitoring
```
- View real-time sensor data
- Monitor engine health
- Track basic metrics

### 2. Performance Testing
```bash
# Start performance testing demo
./obd2tool --demo --scenario=performance_test
```
- 0-60 mph testing
- Quarter mile runs
- Acceleration logging
- Power curves

### 3. Engine Diagnostics
```bash
# Start diagnostics demo
./obd2tool --demo --scenario=diagnostics
```
- Trouble code simulation
- Sensor testing
- System checks
- Health monitoring

### 4. Data Logging
```bash
# Start logging demo
./obd2tool --demo --scenario=data_logging
```
- Multiple format support
- Real-time graphs
- Export capabilities
- Data analysis

## Demo Configuration

### Basic Configuration
```json
{
  "demo_config": {
    "vehicle": "2014 Mustang GT",
    "simulation_rate": 100,
    "add_noise": true
  }
}
```

### Advanced Configuration
```json
{
  "demo_config": {
    "vehicle": {
      "make": "Ford",
      "model": "Mustang GT",
      "year": 2014,
      "engine": "5.0L V8",
      "transmission": "6-speed Manual"
    },
    "simulation": {
      "update_rate": 100,
      "add_noise": true,
      "noise_level": 0.02,
      "sensor_lag": 5
    },
    "features": {
      "enable_video": true,
      "enable_gps": true,
      "enable_telemetry": true
    }
  }
}
```

## Demo Limitations

### Hardware Simulation
- No physical connection required
- Simulated sensor responses
- Emulated protocols

### Performance
- Limited to system resources
- Simplified physics model
- Basic sensor simulation

## Next Steps

### 1. Try Full Version
- Install hardware
- Connect to vehicle
- Access all features

### 2. Explore Features
- Test different vehicles
- Try various scenarios
- Export and analyze data

### 3. Development
- Review API documentation
- Check example code
- Start customizing

## Support

### Documentation
- [Full Documentation](../README.md)
- [API Reference](../api_reference.md)
- [Examples](../examples/README.md)

### Community
- [Discord Server](https://discord.gg/example)
- [Forum](https://forum.example.com)
- [GitHub Issues](https://github.com/yourusername/efi-diagnostic-tool/issues)

### Contact
- Email: support@example.com
- Twitter: @EFIDiagTool
