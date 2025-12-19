# Example Build Script
#
# This script demonstrates how to build the ChampSim tracer with DynamoRIO.
# Adjust DYNAMORIO_HOME to point to your DynamoRIO installation.

# Set DynamoRIO home directory
# Download from: https://github.com/DynamoRIO/dynamorio/releases
# export DYNAMORIO_HOME=/path/to/dynamorio

if [ -z "$DYNAMORIO_HOME" ]; then
    echo "Error: DYNAMORIO_HOME environment variable is not set"
    echo "Please set it to your DynamoRIO installation directory"
    echo "Example: export DYNAMORIO_HOME=/path/to/DynamoRIO-Linux-10.0.0"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake ..

# Build
make -j$(nproc)

echo "Build complete! The tracer library is in: build/libchampsim_tracer.so"
echo ""
echo "Usage example:"
echo "  $DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so -- /bin/ls"
