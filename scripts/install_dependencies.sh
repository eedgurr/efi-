#!/bin/bash

# Installation script for EFI Diagnostic Tool
# Supports Ubuntu/Debian, macOS, and Windows (via WSL)

set -e

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Installing for macOS..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Install dependencies
    brew install cmake opencv qt@6 dotnet

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Installing for Linux..."
    
    # Detect package manager
    if command -v apt &> /dev/null; then
        # Ubuntu/Debian
        echo "Using apt package manager..."
        sudo apt update
        sudo apt install -y cmake build-essential libopencv-dev qt6-base-dev \
                           dotnet-sdk-6.0 libusb-1.0-0-dev

    elif command -v dnf &> /dev/null; then
        # Fedora/RHEL
        echo "Using dnf package manager..."
        sudo dnf install -y cmake gcc-c++ opencv-devel qt6-qtbase-devel \
                          dotnet-sdk-6.0 libusbx-devel

    else
        echo "Unsupported Linux distribution"
        exit 1
    fi

else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install
sudo make install

echo "Installation complete!"
echo "Run 'obd2tool --help' for usage information"
