#!/bin/bash

# Step 1: Go to project root directory
cd "$(dirname "$0")"

# Step 2: Create build directory if it doesn't exist
mkdir -p build
cd build

# Step 3: Run CMake
cmake ..

# Step 4: Build the project
make

# Step 5: Go back to root and run the server binary from there
cd ..
./build/server
