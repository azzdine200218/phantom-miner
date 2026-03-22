#!/bin/bash

echo "[*] Installing dependencies..."

sudo apt-get update
sudo apt-get install -y build-essential cmake git libssl-dev libcurl4-openssl-dev nlohmann-json3-dev

echo "[*] Dependencies installed."
