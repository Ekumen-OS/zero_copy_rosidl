#!/bin/bash

script_location=$(dirname "$(readlink -f "$0")")
script_name=$(basename "$0")
script_name_minus_extension="${script_name%.*}"

export GIT_COMMITTER_NAME="nobody" 
export GIT_COMMITTER_EMAIL="nobody@ekumenlabs.com"
export GIT_AUTHOR_NAME=$GIT_COMMITTER_NAME
export GIT_AUTHOR_EMAIL=$GIT_COMMITTER_EMAIL

# Rename Apex AI packages in iRobot's repository to avoid name collisions 
git apply ${script_location}/${script_name_minus_extension}.rename.patch
git add .
git commit -m "Rename Apex AI packages in iRobot's repository to avoid name collisions"
# See https://github.com/irobot-ros/ros2-performance/pull/143#pullrequestreview-3290054275
git apply ${script_location}/${script_name_minus_extension}.includes.patch
git add .
git commit -m "Add missing includes to avoid compilation errors"
