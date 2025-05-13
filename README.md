# Cashout OBD2 App

This project is an iOS app that includes the following features:

- **OBD2 Diagnostic Functions**: Real-time vehicle diagnostics.
- **Telemetry**: Live data streaming from the vehicle.
- **Race Car Lap Timer**: Track and record lap times.
- **Data Logging**: Save data in CSV format compatible with MegaLogViewerHD.
- **Histogram and 3D Viewing**: Visualize data using scientific computing.
- **MAF Signal Simulation**: Simulate an ECU for testing and analysis.

## Installation

1. Clone the repository:
   ```bash
   git clone <repository-url>
   ```
2. Navigate to the project directory:
   ```bash
   cd cashout-obd2-app
   ```
3. Open the `.xcodeproj` file in Xcode:
   ```bash
   open OBD2App.xcodeproj
   ```
4. Select your target device (iPhone or simulator) in Xcode.
5. Build and run the app:
   - Press `Cmd + R` in Xcode to build and run the app on your selected device.

## Demo

To demo the app:

1. Launch the app on your iPhone or simulator.
2. Explore the following features:
   - **PID Selection**: Choose preferred PIDs from a scrollable list.
   - **PID Calculator**: Calculate and visualize relationships between PIDs (e.g., Engine RPM vs. Vehicle Speed).
   - **Telemetry**: Monitor real-time data from your vehicle.
   - **Lap Timer**: Record and analyze lap times.
   - **Data Logging**: Export logs in CSV format for analysis.
   - **Visualization**: View histograms and 3D plots of performance metrics.
   - **ECU Simulation**: Simulate MAF signals for testing.

## License

This project is licensed under the MIT License.
