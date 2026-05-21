#!/bin/bash
# Apply core_overrides/ packages on top of a ROS 2 source tree.
#
# Usage:
#   apply_core_overrides.sh <overrides_dir> <src_dir> [colcon_ws]
#
# Arguments:
#   overrides_dir  Directory whose immediate subdirectories are package overrides
#                  (e.g. core_overrides/ or /tmp/core_overrides)
#   src_dir        Root of the ROS 2 source tree to patch
#                  (e.g. $ROS_CORE_WS/src)
#   colcon_ws      Optional: path to the colcon workspace root. When provided,
#                  build/ and install/ artifacts for all replaced packages are
#                  cleaned to avoid stale symlink conflicts on rebuild.
#                  (e.g. $ROS_CORE_WS)
#
# Example (inside dev_workspace after bind-mounts are in place):
#   /home/developer/ws/src/project/docker/scripts/apply_core_overrides.sh \
#       /home/developer/ws/src/project/core_overrides \
#       $ROS_CORE_WS/src \
#       $ROS_CORE_WS

set -euo pipefail

OVERRIDES_DIR="${1:?Usage: $0 <overrides_dir> <src_dir> [colcon_ws]}"
SRC_DIR="${2:?Usage: $0 <overrides_dir> <src_dir> [colcon_ws]}"
COLCON_WS="${3:-}"

for override in "$OVERRIDES_DIR"/*/; do
    [ -d "$override" ] || continue
    pkg=$(basename "$override")
    upstream=$(find "$SRC_DIR" -maxdepth 2 -type d -name "$pkg" -print -quit)
    if [ -z "$upstream" ]; then
        echo "ERROR: override package '$pkg' not found under $SRC_DIR"
        exit 1
    fi
    # In dev_workspace the target is already a bind mount — skip source copy
    # but still clean colcon artifacts, since named volumes are initialized with
    # the image's pre-built content and --symlink-install will collide otherwise.
    if mountpoint -q "$upstream" 2>/dev/null; then
        echo "Skipping source copy (already bind-mounted): $pkg"
    else
        echo "Applying override: $pkg -> $(dirname "$upstream")/"
        rm -rf "$upstream"
        cp -r "$override" "$(dirname "$upstream")/$pkg"
    fi

    # Clean colcon artifacts for every ROS package within this override so
    # that --symlink-install does not trip over pre-existing symlinks.
    if [ -n "$COLCON_WS" ]; then
        while IFS= read -r package_xml; do
            colcon_pkg=$(grep -oP '(?<=<name>)[^<]+' "$package_xml" | head -1)
            if [ -n "$colcon_pkg" ]; then
                echo "  Cleaning colcon artifacts for: $colcon_pkg"
                rm -rf "$COLCON_WS/build/$colcon_pkg" "$COLCON_WS/install/$colcon_pkg"
            fi
        done < <(find "$(dirname "$upstream")/$pkg" -name "package.xml")
    fi
done
