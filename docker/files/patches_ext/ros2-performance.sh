#!/bin/bash

script_location=$(dirname "$(readlink -f "$0")")
script_name=$(basename "$0")
script_name_minus_extension="${script_name%.*}"

# see https://github.com/irobot-ros/ros2-performance/pull/143#pullrequestreview-3290054275
git apply ${script_location}/${script_name_minus_extension}.patch
