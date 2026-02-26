# mini-redis

A TCP key-value server written in C++ that implements a subset of the Redis protocol.
Clients connect using `redis-cli` — no custom client needed.

---

## Architecture

```
                    ┌─────────────────────────────────────────┐
  redis-cli ──TCP──►│  epoll event loop  (main thread)        │
  redis-cli ──TCP──►│                                         │
  redis-cli ──TCP──►│  handle_accept()   handle_read()        │
                    │  handle_write()    CommandDispatcher     │
                    │            │                            │
                    │            ▼                            │
                    │       KeyValueStore                      │
                    │     (unordered_map)                      │
                    │            ▲                            │
                    │            │  pending_deletions queue   │
                    │  ExpiryJanitor  (background thread)     │
                    └─────────────────────────────────────────┘
                                 │
                          appendonly.aof  (optional)
```

**Single-threaded event loop** — the same model real Redis uses.
One background thread (the TTL janitor) communicates with the event loop via a mutex-protected queue.
The event loop is the sole writer to the store, so there is zero lock contention on the hot path.

---

## Features

| Command | Syntax | Notes |
|---------|--------|-------|
| PING | `PING [message]` | Returns PONG or the message |
| SET | `SET key value [EX secs] [PX ms]` | Optional expiry |
| GET | `GET key` | Returns nil on miss or expired key |
| DEL | `DEL key [key ...]` | Returns count deleted |
| EXISTS | `EXISTS key` | 1 or 0 |
| EXPIRE | `EXPIRE key seconds` | Set / overwrite TTL |
| TTL | `TTL key` | -1 = no expiry, -2 = missing |
| PERSIST | `PERSIST key` | Remove TTL |
| KEYS | `KEYS pattern` | Glob patterns via fnmatch(3) |
| TYPE | `TYPE key` | Always "string" |
| DBSIZE | `DBSIZE` | Number of keys |
| FLUSHALL | `FLUSHALL` | Delete all keys |
| INFO | `INFO` | Server stats |

**TTL expiry** uses two layers:
- *Lazy*: expired keys are deleted on `GET` (O(1), zero background cost)
- *Active*: a janitor thread samples up to 20 keys every 100 ms and queues deletions — the same probabilistic strategy real Redis uses

**AOF persistence** logs every write command as a RESP array. On restart the file is replayed through the same parser and dispatcher to rebuild state.

> **Known limitation**: `SET foo bar EX 60` is logged as-is. On replay 30 s later the key gets a fresh 60 s TTL instead of the remaining 30 s. The correct fix is to log `EXPIREAT` with an absolute Unix timestamp.

---

## Build

Requires: `cmake >= 3.20`, `g++ / clang++` with C++20 support, Linux (uses epoll).

```bash
# Debug (default)
cmake -B build
cmake --build build -j$(nproc)

# Release
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)

# AddressSanitizer
cmake -B build-asan -DCMAKE_BUILD_TYPE=Asan
cmake --build build-asan -j$(nproc)
```

GoogleTest is fetched automatically by CMake — no system install needed.

---

## Run

```bash
# Default: port 6379, no persistence
./build/mini_redis

# With AOF persistence
./build/mini_redis --aof-enabled --aof-path appendonly.aof --aof-fsync everysecond

# Custom port
./build/mini_redis --port 7379
```

**Flags**

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | 6379 | TCP port |
| `--bind` | 127.0.0.1 | Bind address |
| `--aof-enabled` | false | Enable AOF logging |
| `--aof-path` | appendonly.aof | AOF file path |
| `--aof-fsync` | everysecond | `always` / `everysecond` / `no` |

---

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Covers: RESP parser (partial frames, pipelining), store (TTL, expiry), commands (error inputs), janitor, AOF (write/replay).

---

## Benchmarks

Measured on WSL2 (Linux 6.6, i7), Release build, 50 concurrent clients, 100 000 requests.

```
redis-benchmark -p 6379 -t set,get -c 50 -n 100000 -q
```

| Operation | Throughput | p50 latency |
|-----------|-----------|-------------|
| SET | ~69 500 req/s | 0.35 ms |
| GET | ~75 900 req/s | 0.35 ms |

No AOF. Throughput drops with `--aof-fsync always` (one `flush` per command) and recovers with `everysecond`.

---

## Design notes

- **epoll edge-triggered (EPOLLET)**: fires once per state change; the handler must drain the fd until `EAGAIN`. Level-triggered would be simpler but can stall under load.
- **RESP parser**: never mutates the buffer on partial data — returns `NeedMoreData` so the caller retries when more bytes arrive. Handles TCP fragmentation and pipelining correctly.
- **Write buffering**: responses accumulate in a per-connection `write_buf`. `EPOLLOUT` is registered only when the buffer is non-empty and removed once drained, avoiding spurious wakeups.
- **Thread safety**: `data_` (the store) is touched only by the event loop thread. The janitor holds a `shared_lock` on `expiry_index_` while sampling; the event loop holds a `unique_lock` when writing TTLs.
