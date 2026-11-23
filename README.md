# zero_copy_rosidl

An experimental ROS 2 reference implementation of
[REP-0157](https://reps.openrobotics.org/rep-0157-2026/) (and addendums):
language-specific message views over a runtime-agnostic memory representation,
with constrained loans for variable-size messages. This repository contains a
Jazzy proof of concept and a Rolling port, including ROS core changes, an
XCDRv1 implementation, and benchmarks. Neither is part of a stock ROS 2
installation. See the [Rolling architecture guide](ROLLING_PORT_GUIDE.md) for
the current port's data paths, compatibility boundaries, build instructions,
benchmark reporting, and upstream work.

## Scope

| Area | Implementation |
| --- | --- |
| Message representation | Rolling's native implementation uses an XCDRv1 layout and retains ordinary ROS serialization/copy paths |
| Language views | Experimental C/C++ views and opt-in generated `pybind11`-backed Python handles |
| Constraints | Endpoint-wide bounds and optional tighter per-loan bounds |
| Client libraries | Region topics in `rclcpp` and `rclpy`; C++ region services use explicit polling |
| Middleware | Fast DDS native XCDRv1 topic/service loans; other RMWs can use high-level copy fallback |
| Evaluation | `constrained_pubsub_benchmark` for supported copy and constrained paths across C++ and Python |

Loans are not universal: unsupported layouts, encodings, or middleware
capabilities select an ordinary copy path in the high-level adapters.
Successful delivery over a `shmem_ds` profile does not prove that DDS data
sharing was used or that the whole application avoided copies.

## How the path fits together

```text
generated region view ── rclcpp / rclpy ── rcl ── rmw ── Fast DDS
         │                   │
         └── XCDR type support + xcdr_buffers
         └── ordinary-message conversion when native loans are unavailable
```

The [Rolling architecture guide](ROLLING_PORT_GUIDE.md) has the package map,
native and fallback sequence diagrams, compatibility matrix, known quirks, and
upstream checklist.

## Repository layout

- `code/core/` — pinned ROS 2 and middleware sources imported by
  `code/import.py` from the manifests in `code/core/.repos/`, including the
  experimental message views, constraints, and loan plumbing (under
  `code/core/<distro>/ros2/`).
- `code/rosidl_typesupport_xcdr/` — language-neutral XCDR operation table,
  C++ implementation, and layout buffers.
- `code/constrained_pubsub_benchmark/` — benchmark executables, run drivers,
  Fast DDS profiles, analysis, and tests.
- `docker/` — containerized builds (`Dockerfile`, `docker-compose.yml`, `run.sh`).
- `data/pilot/` — checked-in pilot CSVs, machine-state log, statistics,
  figures, and generated report.

## Build

**TL;DR:** the commands below are for the Jazzy proof of concept. For Rolling,
including its source-built workspace, current manifest pins, and Python region
view generation, follow the [Rolling architecture guide](ROLLING_PORT_GUIDE.md).
For Jazzy, the Docker path is: build the image, import pinned sources once,
build, then smoke-test. For a host build, do the Native import first, then
steps 3–4.

```bash
# 1. Docker (from the repository root): build the image once, then open a
#    shell in ~/ws (code/ is mounted at src, build artifacts persist)
./docker/run.sh --build --service dev_environ -- true
./docker/run.sh -s dev_environ -- bash

# 2. First time only: import the pinned sources (from ~/ws in the container)
./src/import.py

# 3. Build — inside the container, or on the host after
#    `source /opt/ros/jazzy/setup.bash`
cd ~/ws
MAKEFLAGS=-j2 colcon build --executor sequential \
  --packages-up-to constrained_pubsub_benchmark \
  --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash

# 4. Smoke-test
ros2 run constrained_pubsub_benchmark spike
```

### Docker

`docker/` provides Ubuntu Noble + ROS 2 Jazzy images through
`docker/run.sh` (a thin wrapper over `docker compose run`); CI uses the same
services. `dev_environ` (default) mounts the host `code/` tree at `~/ws/src`,
persists build/install/ccache in a named volume, and sizes `/dev/shm` to
2 GB; `patched_ros_environ` and `base_ros_environ` ship pre-built patched and
stock workspaces for comparisons. The mount replaces the image's `src/`, so
run `./src/import.py` at least once (first time, from `~/ws`) to populate the
pinned sources; dependencies are installed when the image is built.

### Native

The commands assume ROS 2 Jazzy on the host and the repository checked out
at `~/ws/src`.

```bash
cd ~/ws/src
source /opt/ros/jazzy/setup.bash
./code/import.py             # first time: pinned sources + patches
```

Then build with step 3 above.

### Test

```bash
cd ~/ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
colcon test --packages-select constrained_pubsub_benchmark \
  --event-handlers console_direct+
colcon test-result --verbose
```

Python tests cover orchestration and analysis; C++ tests cover configuration,
CSV records, and smoke behavior. The `analyze` driver additionally needs the
Python packages `pandas`, `numpy`, and `matplotlib`.

## Run the benchmark

Three drivers, installed as `ros2 run` entry points. All resume via `.done`
markers, honor `RESULTS_DIR` and `DRY_RUN=1`, and must run one at a time;
machine hygiene and sweep options live in the
[benchmark runbook](code/constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md).
These driver matrix sizes and the pilot results below describe the Jazzy
benchmark cases. Rolling supports three cases; use the
[Rolling comparison recipe](ROLLING_PORT_GUIDE.md#benchmark-runs-and-comparable-reports)
and the [runbook](code/constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md#11-rolling-comparison-and-reports)
for a filtered report and the limitations on cross-case comparisons.

```bash
# spike: C++ → C++, constrained publisher, 1 MB @ 10 Hz, best-effort,
# shmem_ds, full bounds, seeded jitter → /tmp/spike_results + analysis/
ros2 run constrained_pubsub_benchmark spike

# pilot: 5 cases × 4 inter-process directions, 100 KB + 1 MB, 30 s steps (~25 min)
RESULTS_DIR=$HOME/benchmarks/pilot ros2 run constrained_pubsub_benchmark pilot
ros2 run constrained_pubsub_benchmark analyze $HOME/benchmarks/pilot --expected-jitter 0.01

# matrix: 5 cases × 6 directions × 2 reliabilities × 7 payloads × 6 freqs
# × 3 transports = 7,560 steps (~63 h publish time); DRY_RUN=1 lists commands
RESULTS_DIR=$HOME/benchmarks/matrix ros2 run constrained_pubsub_benchmark matrix
ros2 run constrained_pubsub_benchmark analyze $HOME/benchmarks/matrix --expected-jitter 0.01
```

## Benchmark cases

The five-case table below describes the Jazzy benchmark. Rolling supports a
different three-case set; see the [Rolling comparison recipe](ROLLING_PORT_GUIDE.md#benchmark-runs-and-comparable-reports).

| Case | Message | Publisher | Subscriber | Type support |
| --- | --- | --- | --- | --- |
| `std_copy_fastcdr` | Standard `sensor_msgs/msg/Image` | Copy | Copy | Fast CDR |
| `std_copy_xcdr` | Standard `sensor_msgs/msg/Image` | Copy | Copy | XCDR |
| `exp_copy_xcdr` | Experimental image | Copy | Copy | XCDR |
| `exp_constrained_pub_only_xcdr` | Experimental image | Constrained loan | Ordinary take | XCDR |
| `exp_constrained_pub_sub_xcdr` | Experimental image | Constrained loan | Constrained loan | XCDR |

Six directions: `cpp_to_cpp`, `cpp_to_py`, `py_to_cpp`, `py_to_py`,
`intra_cpp`, `intra_py` (intra-process cases disable ROS intra-process
delivery so DDS remains in the measured path). Three transports: `udp`
(UDPv4 only), `shmem`, and `shmem_ds` (shared memory with Fast DDS data
sharing on `AUTOMATIC`).

CLI axes, sweep grids, single-run commands, the raw CSV schema, and
troubleshooting: [benchmark runbook](code/constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md).

## Measurement hygiene

At low publication rates, deep CPU C-state wake-ups added ~100–200 µs and
reordered otherwise identical runs; disable deep package C-states
(firmware/OS) before comparable runs — the drivers do not manage them.
Otherwise the drivers fix the known confounders (seeded publish dither,
full-bounds fills, stale SHM cleanup; pilot and matrix also log machine
state and cool down between steps). Run one driver at a time on an idle
host and keep frequency/idle-state settings fixed. The drivers' full hygiene
list and the CSV clock-domain rule are in the
[benchmark runbook](code/constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md).

## Pilot result

Selected 1 MB steps from the checked-in Jazzy pilot under `data/pilot/`: 10 Hz
publication, best-effort reliability, `shmem_ds`, 30 s steps, deterministic
one-percent jitter, full-bounds fills, deep C-states disabled. Each row
contains 301 matched samples with no drops.

| Data path | Direction | p50 | p95 |
| --- | --- | ---: | ---: |
| Constrained publisher | C++ → C++ | 111.147 µs | 176.340 µs |
| Constrained publisher and subscriber | C++ → C++ | 129.013 µs | 199.385 µs |
| Experimental XCDR copy | C++ → C++ | 488.014 µs | 1.034 ms |
| Standard XCDR copy | C++ → C++ | 655.769 µs | 1.164 ms |
| Standard Fast CDR copy | C++ → C++ | 864.070 µs | 1.420 ms |

These numbers describe one machine and one configuration: evidence for the
reference implementation, not portable latency guarantees. Use the raw CSVs
and `machine_state.log` when comparing results.

## Further reading

- [REP-0157](https://reps.openrobotics.org/rep-0157-2026/)
- [REP-0157 addendum](https://github.com/openrobotics/reps/pull/28)
- [Benchmark runbook](code/constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md)
