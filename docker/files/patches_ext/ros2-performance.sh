#!/bin/bash

script_location=$(dirname "$(readlink -f "$0")")
script_name=$(basename "$0")
script_name_minus_extension="${script_name%.*}"

touch COLCON_IGNORE
