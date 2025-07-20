#!/bin/bash

echo "🔧 Installing dependencies..."

# Update package list
sudo apt update

# Install g++, curl dev libs, and make
sudo apt install -y g++ make libcurl4-openssl-dev

echo "✅ All dependencies installed!"
