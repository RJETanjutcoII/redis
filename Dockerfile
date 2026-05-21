# ── Stage 1: build ────────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake g++ make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy cmake config before source so this layer is cached independently.
# CMake configure does not compile anything, so source files don't need to
# exist yet. Changing a .cpp file will NOT re-run cmake or re-download deps.
COPY CMakeLists.txt .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

# Copy source — only this layer and the build step below are invalidated on
# source changes.
COPY src/ src/
RUN cmake --build build --target mini_redis -j$(nproc)

# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM debian:bookworm-slim

COPY --from=builder /src/build/mini_redis /usr/local/bin/mini_redis

EXPOSE 6379
VOLUME ["/data"]
WORKDIR /data

ENTRYPOINT ["mini_redis"]
CMD ["--port", "6379", "--bind", "0.0.0.0"]
