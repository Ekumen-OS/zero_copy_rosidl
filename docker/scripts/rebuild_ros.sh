#!/bin/bash
# Apply core_overrides/ and rebuild the ROS core workspace.
# Intended to be run from inside dev_workspace after modifying core_overrides/.
#
# Usage:
#   rebuild_ros.sh [-- COLCON_ARGS...]
#
# Examples:
#   rebuild_ros.sh
#   rebuild_ros.sh -- --packages-up-to my_package
#   rebuild_ros.sh -- --packages-select rosidl_generator_py  # safe only for leaf/Python-only packages;
#                                                             # apply_core_overrides cleans all override
#                                                             # artifacts, so C/C++ deps must be rebuilt too

set -euo pipefail

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
PROJECT_DIR=$(readlink -f "$SCRIPT_DIR/../..")

OVERRIDES_DIR="$PROJECT_DIR/core_overrides"
WS="${ROS_CORE_WS:?ROS_CORE_WS is not set - are you inside the container?}"

# Default colcon args, overridable via -- ...
COLCON_ARGS=(
    --symlink-install
    --cmake-args -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    --packages-up-to performance_test
)

# Parse: everything after -- overrides the default colcon args
while [[ $# -gt 0 ]]; do
    case "$1" in
    --)
        shift
        COLCON_ARGS=("$@")
        break
        ;;
    *)
        echo "Unknown argument: $1"
        echo "Usage: $(basename "$0") [-- COLCON_ARGS...]"
        exit 1
        ;;
    esac
done

echo "==> Applying core_overrides/ to $WS/src"
"$SCRIPT_DIR/apply_core_overrides.sh" "$OVERRIDES_DIR" "$WS/src" "$WS"

echo "==> Building: colcon build ${COLCON_ARGS[*]}"
cd "$WS"
colcon build "${COLCON_ARGS[@]}"
