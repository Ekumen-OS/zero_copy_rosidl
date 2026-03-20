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
