#!/bin/bash

# Demo script for EFI Diagnostic Tool

# Default values
VEHICLE="2014 Mustang GT"
SCENARIO="basic_monitoring"
CONFIG_FILE="config/demo_config.json"
UPDATE_RATE=100
NOISE_LEVEL=0.02

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --vehicle=*)
            VEHICLE="${1#*=}"
            shift
            ;;
        --scenario=*)
            SCENARIO="${1#*=}"
            shift
            ;;
        --config=*)
            CONFIG_FILE="${1#*=}"
            shift
            ;;
        --update-rate=*)
            UPDATE_RATE="${1#*=}"
            shift
            ;;
        --noise=*)
            NOISE_LEVEL="${1#*=}"
            shift
            ;;
        --help)
            echo "EFI Diagnostic Tool Demo"
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --vehicle=<vehicle>    Specify vehicle (default: 2014 Mustang GT)"
            echo "  --scenario=<scenario>  Specify demo scenario"
            echo "  --config=<file>        Use custom config file"
            echo "  --update-rate=<hz>     Set update rate in Hz"
            echo "  --noise=<level>        Set noise level (0-1)"
            echo "  --help                 Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create demo config
cat > "$CONFIG_FILE" << EOF
{
    "demo_config": {
        "vehicle": {
            "make": "Ford",
            "model": "Mustang GT",
            "year": 2014,
            "engine": "5.0L V8",
            "transmission": "6-speed Manual",
            "weight": 3700,
            "performance": {
                "max_hp": 420,
                "max_torque": 390,
                "redline": 7000,
                "max_boost": 25,
                "gear_ratios": [3.66, 2.43, 1.69, 1.32, 1.00, 0.65]
            }
        },
        "simulation": {
            "update_rate": $UPDATE_RATE,
            "add_noise": true,
            "noise_level": $NOISE_LEVEL,
            "sensor_lag": 5,
            "scenario": "$SCENARIO"
        },
        "features": {
            "enable_video": true,
            "enable_gps": true,
            "enable_telemetry": true,
            "enable_logging": true
        }
    }
}
EOF

# Run demo
echo "Starting EFI Diagnostic Tool Demo"
echo "Vehicle: $VEHICLE"
echo "Scenario: $SCENARIO"
echo "Update Rate: $UPDATE_RATE Hz"

# Launch application in demo mode
./obd2tool --demo --config="$CONFIG_FILE"
