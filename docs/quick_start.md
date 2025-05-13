# Quick Start Guide

## Demo Version

The demo version simulates a 2014 Mustang GT with realistic sensor data and responses.

### Running the Demo

1. Download the demo:
```bash
git clone https://github.com/yourusername/efi-diagnostic-tool.git
cd efi-diagnostic-tool
```

2. Run the demo:
```bash
./obd2tool --demo
```

### Demo Features

- Simulated OBD2 data
- Engine health monitoring
- Performance metrics
- Data logging
- Video capture simulation
- GPS tracking simulation

## Full Application

### Prerequisites

- CMake 3.15+
- C++17 compiler
- .NET 6.0+ (for MAUI interface)
- OpenCV (for video capture)
- Qt 6.2+ (for desktop UI)

### Installation

1. Install dependencies:
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake build-essential libopencv-dev qt6-base-dev

# macOS
brew install cmake opencv qt@6

# Windows
# Use the installer from the releases page
```

2. Build the application:
```bash
mkdir build && cd build
cmake ..
make
```

3. Install:
```bash
sudo make install
```

### First Run

1. Connect your OBD2 adapter
2. Launch the application:
```bash
obd2tool
```

3. Select your device type and connection method
4. Click "Connect" to begin monitoring

### Basic Usage

1. **Real-time Monitoring**
   - View live sensor data
   - Monitor engine health
   - Track performance metrics

2. **Data Logging**
   - Start/stop logging sessions
   - Export data in multiple formats
   - Compare sessions

3. **Video Recording**
   - Record drives with telemetry overlay
   - Capture GPS tracks
   - Export combined data

## Troubleshooting

### Common Issues

1. **Connection Problems**
   - Verify adapter is powered
   - Check COM port settings
   - Ensure proper drivers are installed

2. **Performance Issues**
   - Lower sample rates
   - Disable video capture
   - Check system resources

### Getting Help

- Check the [FAQ](faq.md)
- Visit our [Forum](https://forum.example.com)
- Submit issues on [GitHub](https://github.com/yourusername/efi-diagnostic-tool/issues)

## Next Steps

- Read the [User Manual](user_manual.md)
- Try the [Tutorials](tutorials/README.md)
- Join our [Discord](https://discord.gg/example)
