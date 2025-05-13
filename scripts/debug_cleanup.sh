#!/bin/bash

# Debug cleanup script for EFI Diagnostic Tool

set -e

echo "Starting debug cleanup..."

# Clean build directories
echo "Cleaning build directories..."
rm -rf build/
rm -rf bin/
find . -name "*.o" -type f -delete
find . -name "*.obj" -type f -delete
find . -name "*.pdb" -type f -delete

# Clean logs
echo "Cleaning log files..."
find . -name "*.log" -type f -delete
find . -name "*.xdf" -type f -delete
find . -name "*.a2l" -type f -delete

# Clean temporary files
echo "Cleaning temporary files..."
find . -name "*~" -type f -delete
find . -name ".DS_Store" -type f -delete
find . -name "Thumbs.db" -type f -delete

# Clean debug symbols
echo "Cleaning debug symbols..."
find . -name "*.dSYM" -type d -exec rm -r {} +
find . -name "*.pdb" -type f -delete

# Clean demo recordings
echo "Cleaning demo recordings..."
find . -name "demo_*.mp4" -type f -delete
find . -name "demo_*.csv" -type f -delete

# Verify git status
echo "Checking git status..."
git status

# Report space saved
echo "Space cleaned up:"
df -h .

echo "Debug cleanup complete!"
