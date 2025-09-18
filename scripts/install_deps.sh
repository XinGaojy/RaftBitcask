#!/bin/bash

# Bitcask C++ 依赖安装脚本

set -e

echo "Installing Bitcask C++ dependencies..."

# 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command -v apt-get &> /dev/null; then
        # Ubuntu/Debian
        echo "Detected Ubuntu/Debian system"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            libcrc32c-dev \
            zlib1g-dev \
            pkg-config
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL
        echo "Detected CentOS/RHEL system"
        sudo yum install -y \
            gcc-c++ \
            cmake \
            crc32c-devel \
            zlib-devel \
            pkgconfig
    elif command -v dnf &> /dev/null; then
        # Fedora
        echo "Detected Fedora system"
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            crc32c-devel \
            zlib-devel \
            pkgconfig
    else
        echo "Unsupported Linux distribution"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Detected macOS system"
    if command -v brew &> /dev/null; then
        brew install cmake crc32c zlib pkg-config
    else
        echo "Please install Homebrew first: https://brew.sh/"
        exit 1
    fi
else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

echo "Dependencies installed successfully!"
echo "You can now build the project with:"
echo "  make build"
echo "or"
echo "  mkdir build && cd build && cmake .. && make"
