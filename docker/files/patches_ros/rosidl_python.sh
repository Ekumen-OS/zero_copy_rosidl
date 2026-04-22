#!/bin/bash

script_location=$(dirname "$(readlink -f "$0")")
script_name=$(basename "$0")
script_name_minus_extension="${script_name%.*}"

git apply --whitespace=fix ${script_location}/${script_name_minus_extension}.patch
