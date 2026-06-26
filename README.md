# zero_copy_rosidl

## Overview

This repository is a Proof-of-Concept (PoC) implementation of zero-copy ROSIDL message serialization and deserialization in ROS 2. It serves as a reference implementation for [REP-0157](https://reps.openrobotics.org/rep-0157-2026/), demonstrating the feasibility and performance benefits of zero-copy communication in ROS 2.

Further reads:

- https://github.com/openrobotics/reps/pull/22
- https://github.com/openrobotics/reps/pull/28

<p align="center">
  <img src=".images/under_construction.png" alt="Under construction banner" width="400">
</p>


## Development Environment

### Quick Start

Launch the development container:

```bash
./docker/run.sh [--build] [-s unpatched_workspace|patched_workspace|dev_workspace] [-- COMMAND [ARGS...]]
```

Use `--build` to rebuild the image. Services:
- `unpatched_workspace` — stock ROS 2 Jazzy build
- `patched_workspace` — `core_overrides/` applied on top (used for CI and reproducible benchmarks)
- `dev_workspace` (default) — same as `patched_workspace` but with persistent named volumes for `build/` and `install/`, enabling fast incremental rebuilds; `core_overrides/` is accessible via the repo bind-mount so host edits are picked up immediately by `rebuild_ros.sh`

Commands after `--` execute inside the container.

### Modifying ROS Core Packages

Modifications to ROS 2 core packages live in `core_overrides/`. Each subdirectory mirrors the upstream package name (e.g. `core_overrides/rosidl/`, `core_overrides/rosidl_python/`). When `rebuild_ros.sh` runs, it copies each override on top of the corresponding upstream package in the ROS core workspace.

After modifying files in `core_overrides/`, rebuild from inside the container using the convenience script:

```bash
# Inside dev_workspace — rebuilds everything up to performance_test, rosidl_cli, ros2cli, ros2run
~/ws/src/project/docker/scripts/rebuild_ros.sh
```

This applies the overrides and runs `colcon build --symlink-install --packages-up-to performance_test rosidl_cli ros2cli ros2run` by default. To target specific packages instead:

```bash
~/ws/src/project/docker/scripts/rebuild_ros.sh -- --symlink-install --packages-select <package_name>
```

Build artifacts are stored in named Docker volumes (`zero_rosidl_dev_build`, `zero_rosidl_dev_install`) and persist across container restarts.

### Accessing Running Container

To open a shell in a running container:

```bash
docker exec -it <username>_<service_name> bash
```

Replace `<username>` with your system username and `<service_name>` with the running service (e.g. `ekumen_unpatched_workspace` or `ekumen_dev_workspace`).

## Running Performance Benchmarks

This project uses [ApexAI's performance_test](https://gitlab.com/ApexAI/performance_test)
to benchmark ROS 2 communication performance under zero-copy implementations.

### Generating Benchmark Data

The testing frameworks are available instantly due to the multi-stage Docker builds. Inside `unpatched_workspace` and `patched_workspace` the ApexAI performance test suite is already compiled. In `dev_workspace`, run `rebuild_ros.sh` after first launch to build the workspace into the persistent volumes.

```bash
# Inside the container, source the workspace and run a performance test
cd $ROS_CORE_WS
source install/setup.bash
ros2 run performance_test perf_test --help
```

### Viewing Benchmark Results

Results are written to a JSON log file via the `-l` flag. Use the bundled parser script to print a summary:

```bash
# Example: run a 30-second publisher/subscriber test and write results to a JSON file
ros2 run performance_test perf_test -c rclcpp-single-threaded-executor -m Array1k -t test_topic -s 1 -p 1 --max-runtime 30 -l /tmp/results.json

# Parse and display key metrics
python3 ~/ws/src/project/docker/scripts/parse_benchmark_results.py /tmp/results.json
```
