# zero_copy_rosidl

An experimental ROS 2 reference implementation of [REP-0157](https://reps.openrobotics.org/rep-0157-2026/)
(and addendums): language-specific message views over a runtime-agnostic
memory representation, with constrained loans for variable-size messages. It
provides the proposed ROS core changes, an XCDRv1 type support, and a
benchmark comparing copy paths with constrained publisher and subscriber loans
in C++ and Python. Not part of a stock ROS 2 Jazzy installation.

## Scope

| Area | Implementation |
| --- | --- |
| Message representation | XCDRv1 plain CDR used as both the transferable form and external-storage layout |
| Language views | Experimental C++ messages and generated `pybind11`-backed Python handles |
| Constraints | Endpoint-wide bounds and optional tighter per-loan bounds |
| Client libraries | Constrained publisher and subscription plumbing in `rclcpp` and `rclpy` |
| Middleware | `rmw_fastrtps_cpp` integration with Fast DDS loans, shared-memory transport, and data sharing |
| Evaluation | `constrained_pubsub_benchmark` for copy and constrained paths across C++ and Python |

Loans are not universal: unsupported layouts, encodings, or middleware
capabilities fall back to an ordinary copy path. Topics are the tested path;
services are not demonstrated to the same extent.

## How the path fits together

```text
generated experimental message view
               │
       rclcpp / rclpy API
               │
     constrained rcl and rmw APIs
               │
 rmw_fastrtps XcdrTypeSupport adapter
               │
  Fast DDS loaned storage and transport
```

A constrained publisher supplies upper bounds for every variable-size member;
the XCDR type support computes the storage from those bounds and builds the
view directly over a Fast DDS loan, compacting in place when the message is
smaller, then writing the same middleware-owned block. On receive, Fast DDS
loans the sample, the type support casts it into a C++ view, and `rclcpp` and
`rclpy` keep that loan alive for the callback; Python wraps the same view with
a generated `MessageTypeBridge` — there is no separate CPython XCDR type
support package.

Endpoint constraints set the allocation and validation baseline; a per-loan
constraint may tighten it for one borrow or take but never loosen it. The
current `rclcpp` and `rclpy` executors take against the subscription baseline
and add no per-take constraints.

## Repository layout

- `code/core/` — pinned ROS 2 and middleware sources imported by
  `code/import.sh` from the manifests in `code/core/.repos/`, including the
  experimental message views, constraints, and loan plumbing (under
  `code/core/ros2/`).
- `code/rosidl_typesupport_xcdr/` — language-neutral XCDR operation table,
  C++ implementation, and layout buffers.
- `code/constrained_pubsub_benchmark/` — benchmark executables, run drivers,
  Fast DDS profiles, analysis, and tests.
- `docker/` — containerized builds (`Dockerfile`, `docker-compose.yml`, `run.sh`).
- `data/pilot/` — checked-in pilot CSVs, machine-state log, statistics,
  figures, and generated report.

## Build

**TL;DR:** Docker path — build the image, import the pinned sources once,
build, then smoke-test. For a host build, do the Native import first, then
steps 3–4.

```bash
# 1. Docker (from the repository root): build the image once, then open a
#    shell in ~/ws (code/ is mounted at src, build artifacts persist)
./docker/run.sh --build --service dev_environ -- true
./docker/run.sh -s dev_environ -- bash

# 2. First time only: import the pinned sources (from ~/ws in the container)
./src/import.sh

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
run `./src/import.sh` at least once (first time, from `~/ws`) to populate the
pinned sources; dependencies are installed when the image is built.

### Native

The commands assume ROS 2 Jazzy on the host and the repository checked out
at `~/ws/src`.

```bash
cd ~/ws/src
source /opt/ros/jazzy/setup.bash
./code/import.sh             # first time: pinned sources + patches
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

Selected 1 MB steps from the checked-in pilot under `data/pilot/`: 10 Hz
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
