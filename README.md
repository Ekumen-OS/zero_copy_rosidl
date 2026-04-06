# zero_copy_rosidl

## Development Environment

### Quick Start

Set the ROS domain ID and launch the development container:

```bash
export ROS_DOMAIN_ID=<your_domain_id>
./docker/run.sh [--build] [-- COMMAND [ARGS...]]
```

Use `--build` to rebuild the image. Commands after `--` execute inside the container.

### Modifying ROS Core Packages

Modifications are applied during the Docker build when ROS core packages are compiled from source. The script-based approach allows for arbitrary changes beyond simple patches, including removing packages, adding COLCON_IGNORE files, or managing git submodules.

1. Create a shell script `docker/files/patches_ros/<package_name>.sh` with your modifications
2. For simple patches, reference a corresponding `.patch` file:

```bash
#!/bin/bash
script_location=$(dirname "$(readlink -f "$0")")
script_name=$(basename "$0")
script_name_minus_extension="${script_name%.*}"
git apply ${script_location}/${script_name_minus_extension}.patch
```

3. For other modifications, implement custom logic directly in the script
4. Make the script executable: `chmod +x docker/files/patches_ros/<package_name>.sh`

The script executes automatically during the build phase when processing the corresponding ROS core package. The same mechanism applies to external dependencies using `docker/files/patches_ext/`.

### Accessing Running Container

To open a shell in a running container:

```bash
docker exec -it <username>_zero_rosidl_devel bash
```

Replace `<username>` with your system username (e.g., `ekumen_zero_rosidl_devel`).

## Running Performance Benchmarks

This project includes [iRobot's ros2-performance](https://github.com/irobot-ros/ros2-performance)
benchmark suite to measure ROS 2 communication performance.

### Building the Workspace

The benchmark packages are built automatically during the Docker image build.
To rebuild the workspace inside a running container:

```bash
# Inside the container, rebuild the workspace
cd $USER_WS
rm -rf build/* install/*
source /opt/ros/jazzy/setup.bash
colcon build
```

### Running the Benchmark

To run the default Sierra Nevada topology benchmark:

```bash
# Inside the container, run the benchmark
cd $USER_WS
source $USER_WS/install/setup.bash
cd ./install/irobot_benchmark/lib/irobot_benchmark
./irobot_benchmark topology/sierra_nevada.json
```

### Viewing Benchmark Results

Results are saved in the `sierra_nevada_log/` directory:

- **`latency_total.txt`**: Summary statistics (mean latency, late messages, lost messages)
- **`latency_all.txt`**: Detailed per-message latency data
- **`metadata.txt`**: Benchmark configuration parameters
- **`resources.txt`**: CPU and memory usage during the test

To view the summary results:

```bash
cat $USER_WS/install/irobot_benchmark/lib/irobot_benchmark/sierra_nevada_log/latency_total.txt
```

Example output:
```
received_msgs  mean_us   late_msgs late_perc too_late_msgs  too_late_perc  lost_msgs lost_perc
5278           131       0         0         0              0              0         0
```

### Available Topology Configurations

The benchmark includes several predefined topologies in the `topology/` directory:

- `sierra_nevada.json` - Default topology (10 nodes, complex graph)
- `mont_blanc.json` - Large topology
- `cedar.json` - Medium complexity
- `white_mountain.json` - High complexity
- `debug_*.json` - Smaller topologies for testing

To run a different topology:

```bash
./irobot_benchmark topology/<topology_name>.json
```
