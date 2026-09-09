# zero_copy_rosidl

An experimental ROS 2 reference implementation of REP-0157: language-specific message views over a runtime-agnostic memory representation, with constrained loans for variable-size messages.

The repository contains the proposed ROS core changes, an XCDRv1 type support implementation, and a benchmark that compares ordinary copy paths with constrained publisher and subscriber loans in C++ and Python.

The design is documented in [REP-0157](https://reps.openrobotics.org/rep-0157-2026/) (and addendums). It remains experimental and is not part of a stock ROS 2 Jazzy installation.

## Scope

The reference implementation currently covers:

| Area | Implementation |
| --- | --- |
| Message representation | XCDRv1 plain CDR used as both the transferable form and external-storage layout |
| Language views | Experimental C++ messages and generated `pybind11`-backed Python handles |
| Constraints | Endpoint-wide bounds and optional tighter per-loan bounds |
| Client libraries | Constrained publisher and subscription plumbing in `rclcpp` and `rclpy` |
| Middleware | `rmw_fastrtps_cpp` integration with Fast DDS loans, shared-memory transport, and data sharing |
| Evaluation | `constrained_pubsub_benchmark` for copy and constrained paths across C++ and Python |

The implementation does not make zero-copy universal. Unsupported message layouts, encodings, or middleware capabilities may reject a loan or use an ordinary copy path. Topic communication is the tested path; service support is not yet demonstrated to the same extent.

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

A constrained publisher supplies upper bounds for every variable-size member. The XCDR type support uses those bounds to calculate the required storage and construct the message view directly over a Fast DDS loan. Publishing compacts the view in place when the actual message is smaller than its bounds, then writes the same middleware-owned block.

On receive, Fast DDS loans the sample and the XCDR type support casts its representation into a C++ view. `rclcpp` and `rclpy` keep that loan alive for the callback and return it afterwards. Python uses a generated `MessageTypeBridge` to wrap the same C++ view in a non-owning Python object; there is no separate CPython XCDR type support package.

Endpoint constraints and per-loan constraints serve different purposes. Endpoint constraints establish the allocation and validation baseline. A per-loan constraint may tighten that baseline for one borrow or take, but may not loosen it. The current `rclcpp` and `rclpy` executors take against the subscription baseline and do not add per-take constraints.

## Repository layout

- `code/core/ros2/` — ROS 2 packages modified for experimental message views, constraints, and loan plumbing.
- `code/rosidl_typesupport_xcdr/` — language-neutral XCDR operation table, C++ implementation, and layout buffers.
- `code/constrained_pubsub_benchmark/` — benchmark executables, run drivers, Fast DDS profiles, analysis, and tests.
- `code/import.sh` (alongside `code/core/.repos/` — source import manifests for the ROS 2 workspace.
- `data/pilot/` — controlled pilot CSVs, machine-state log, statistics, figures, and generated report.

## Build

The repository is intended to live at the `src/` root of a colcon workspace. The commands below assume ROS 2 Jazzy and the repository at `~/ws/src`.

If `core/` has not been populated, import the pinned source repositories first:

```bash
cd ~/ws/src
source /opt/ros/jazzy/setup.bash
./import.sh
```

Build the implementation and benchmark in release mode:

```bash
cd ~/ws
source /opt/ros/jazzy/setup.bash
MAKEFLAGS=-j2 colcon build --executor sequential \
  --packages-up-to constrained_pubsub_benchmark \
  --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
```

The analysis command also requires Python packages `pandas`, `numpy`, and `matplotlib`.

## Run the benchmark

### Spike

Start with the single focused case:

```bash
cd ~/ws
source install/setup.bash
ros2 run constrained_pubsub_benchmark spike
```

The spike runs C++ to C++, a constrained publisher, a 1 MB payload at 10 Hz, best-effort reliability, Fast DDS shared memory with data sharing, full-bounds fill, and deterministic publish jitter. It writes raw data to `/tmp/spike_results` and generates `/tmp/spike_results/analysis/report.md` plus CSV summaries and figures.

Set `RESULTS_DIR` to keep results elsewhere:

```bash
RESULTS_DIR=$HOME/benchmarks/spike \
  ros2 run constrained_pubsub_benchmark spike
```

### Pilot

The pilot covers five cases, four inter-process language directions, 100 KB and 1 MB payloads, and one 10 Hz best-effort shared-memory data-sharing configuration:

```bash
RESULTS_DIR=$HOME/benchmarks/pilot \
  ros2 run constrained_pubsub_benchmark pilot

ros2 run constrained_pubsub_benchmark analyze \
  $HOME/benchmarks/pilot --expected-jitter 0.01
```

It runs 20 sweep invocations, each containing two 30 second steps. Completed invocations receive `.done` markers and are skipped when the driver resumes.

### Full matrix

The matrix covers five cases, six language/process directions, two reliability policies, seven payload sizes, six frequencies, and three Fast DDS transport modes:

```bash
RESULTS_DIR=$HOME/benchmarks/matrix \
  ros2 run constrained_pubsub_benchmark matrix

ros2 run constrained_pubsub_benchmark analyze \
  $HOME/benchmarks/matrix --expected-jitter 0.01
```

The matrix contains 7,560 sequential steps. At 30 seconds per step, publish time alone is about 63 hours; discovery, process setup, and cooldown add further time. Use `DRY_RUN=1` to inspect the commands without executing them.

## Benchmark cases

| Case | Message | Publisher | Subscriber | Type support |
| --- | --- | --- | --- | --- |
| `std_copy_fastcdr` | Standard `sensor_msgs/msg/Image` | Copy | Copy | Fast CDR |
| `std_copy_xcdr` | Standard `sensor_msgs/msg/Image` | Copy | Copy | XCDR |
| `exp_copy_xcdr` | Experimental image | Copy | Copy | XCDR |
| `exp_constrained_pub_only_xcdr` | Experimental image | Constrained loan | Ordinary take | XCDR |
| `exp_constrained_pub_sub_xcdr` | Experimental image | Constrained loan | Constrained loan | XCDR |

Directions are `cpp_to_cpp`, `cpp_to_py`, `py_to_cpp`, `py_to_py`, `intra_cpp`, and `intra_py`. The intra-process cases deliberately disable ROS intra-process delivery so that DDS remains in the measured path.

Transport modes are:

- `udp` — UDPv4 only, data sharing off.
- `shmem` — shared-memory transport, data sharing off.
- `shmem_ds` — shared-memory transport with Fast DDS data sharing set to `AUTOMATIC`.

See [`constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md`](constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md) for individual commands, the raw CSV schema, all sweep options, and troubleshooting notes.

## Reproducible measurements

Low-rate latency is sensitive to host power management. During investigation, deep CPU C-state wake-ups added roughly 100–200 µs and changed the apparent ordering of otherwise identical runs. Disable deep package C-states with the platform's firmware or operating-system controls before collecting comparable data. The benchmark drivers do not change C-state policy.

All three drivers control the benchmark process in the following ways:

- `ROS_DISABLE_LOANED_MESSAGES=0` enables the subscriber loan path.
- One-percent seeded publish-period dither avoids repeatedly aligning samples with the DDS heartbeat grid.
- Full-bounds fills keep the constrained publish case from measuring an unrelated compaction rewrite.
- Stale Fast DDS shared-memory segments are removed before each invocation.

The pilot and matrix drivers also record and control longer-run machine state:

- Temperature, CPU frequency, load, and `/dev/shm` use are appended to `machine_state.log`.
- Thermal cooldown runs between invocations.

The spike performs one SHM cleanup but does not write `machine_state.log` or wait for cooldown.

Run one driver at a time. Avoid background workloads, record any host-level frequency or idle-state settings, and keep those settings unchanged across comparisons.

The CSV uses `CLOCK_MONOTONIC` for application send and receive timestamps. Middleware source and receive timestamps use wall clock. Do not subtract timestamps from different clock domains.

## Pilot result

The table below selects the 1 MB steps from the checked-in pilot under `data/pilot/`. They used 10 Hz publication, best-effort reliability, `shmem_ds`, 30 second steps, deterministic one-percent jitter, full-bounds fills, and disabled deep C-states. Each row contains 301 matched samples with no drops:

| Data path | Direction | p50 | p95 |
| --- | --- | ---: | ---: |
| Constrained publisher | C++ → C++ | 111.147 µs | 176.340 µs |
| Constrained publisher and subscriber | C++ → C++ | 129.013 µs | 199.385 µs |
| Experimental XCDR copy | C++ → C++ | 488.014 µs | 1.034 ms |
| Standard XCDR copy | C++ → C++ | 655.769 µs | 1.164 ms |
| Standard Fast CDR copy | C++ → C++ | 864.070 µs | 1.420 ms |

These numbers describe one machine and one configuration. Use the raw CSVs and `machine_state.log` when comparing results; they are evidence for the reference implementation, not portable latency guarantees.

## Test

```bash
cd ~/ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
colcon test --packages-select constrained_pubsub_benchmark \
  --event-handlers console_direct+
colcon test-result --verbose
```

The benchmark package includes Python tests for orchestration and analysis, plus C++ tests for configuration, CSV records, and smoke coverage.

## Further reading

- [REP-0157](https://reps.openrobotics.org/rep-0157-2026/)
- [REP-0157 addendum](https://github.com/openrobotics/reps/pull/28)
- [Benchmark runbook](constrained_pubsub_benchmark/BENCHMARK_RUNBOOK.md)
