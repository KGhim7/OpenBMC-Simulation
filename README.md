# MiniSim-BMC

A lightweight C++ simulation of a BMC-style telemetry stack with four daemons:
- `busd`: central sensor message bus over TCP
- `sensord`: simulated sensor producer
- `eventd`: threshold-based event/alert consumer
- `redfishd`: minimal Redfish-like HTTP interface

The project is useful for learning daemon-to-daemon communication, event handling, and exposing system telemetry over REST endpoints.

---

## Features

- Real-time simulated sensors (`temp_cpu`, `fan_rpm`, `vcore_cpu`)
- TCP message bus with publish and query (`SENSORS|...`, `GET|ALL`)
- Event detection with start/clear logging for abnormal conditions
- Minimal Redfish-style API for system and thermal resources
- CMake-based build for all daemons 

---

## Project Structure

```text
MiniSim-BMC/
├── CMakeLists.txt
├── run_demo_terminals.sh
├── apps/
│   ├── CMakeLists.txt
│   ├── busd/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── tcp_server.cpp
│   ├── sensord/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   ├── sensor.cpp
│   │   ├── sensor.hpp
│   │   └── tcp_client.cpp
│   ├── eventd/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── eventd.cpp
│   └── redfishd/
│       ├── CMakeLists.txt
│       └── redfishd.cpp
└── build/ (generated)
```

---

## Build

### Requirements

- CMake `>= 3.20`
- C++ compiler with C++20 support (C++17 is enough for `redfishd`)

### Configure and compile

```bash
cmake -S . -B build
cmake --build build -j
```

Generated binaries:
- `build/apps/busd/busd`
- `build/apps/sensord/sensord`
- `build/apps/eventd/eventd`
- `build/apps/redfishd/redfishd`

---

## Running the Simulation

Start processes in this order (separate terminals):

```bash
./build/apps/busd/busd
./build/apps/sensord/sensord
./build/apps/eventd/eventd
./build/apps/redfishd/redfishd
```

Or on macOS, use the provided helper script to launch all components:

```bash
./run_demo_terminals.sh
```

> `run_demo_terminals.sh` uses AppleScript (`osascript`) and opens Terminal windows.

---

## Internal Protocol (busd)

Clients register using:

```text
HELLO|<CLIENT_NAME>
```

Sensor updates are published as:

```text
SENSORS|temp_cpu=<value>;fan_rpm=<value>;vcore_cpu=<value>
```

Snapshot query:

```text
GET|ALL
```

Response format:

```text
SENSORS|key=value;key=value;...
```

---

## Redfish-like Endpoints

Base URL: `http://127.0.0.1:8000`

- `GET /redfish/v1`
- `GET /redfish/v1/Systems`
- `GET /redfish/v1/Systems/1`
- `GET /redfish/v1/Chassis`
- `GET /redfish/v1/Chassis/1`
- `GET /redfish/v1/Chassis/1/Thermal`

Quick check:

```bash
curl -s http://127.0.0.1:8000/redfish/v1/Chassis/1/Thermal
```

---

## Ports Used

| Daemon | Port | Protocol | Purpose |
|---|---:|---|---|
| `busd` | `5555` | TCP | Internal message bus for sensor publish/query |
| `sensord` | `5555` | TCP client -> `busd` | Sends sensor readings to bus |
| `eventd` | `5555` | TCP client -> `busd` | Subscribes to sensor stream and raises alerts |
| `redfishd` | `8000` | HTTP | Serves Redfish-like API endpoints |
| `redfishd` (to `busd`) | `5555` | TCP client -> `busd` | Fetches latest sensor snapshot (`GET|ALL`) |

---

## Event Thresholds (`eventd`)

`eventd` logs transitions when values cross thresholds:

- `temp_cpu > 80.0`  -> high temperature alert
- `vcore_cpu > 4.8`  -> high voltage warning
- `fan_rpm < 600.0`  -> low fan RPM warning

Events are emitted on start and clear transitions.

---

## Notes

- `sensord` sends updates every second.
- `busd` keeps latest sensor values in memory and serves them to other clients.
- `redfishd` currently supports only `GET` requests.
