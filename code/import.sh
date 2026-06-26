#!/bin/bash

set -o errexit

SCRIPT_DIR=$(realpath --relative-to=$(pwd) $(dirname "$(readlink -f "$0")"))

CORE_DIR=$SCRIPT_DIR/core
CORE_REPOS_DIR=$CORE_DIR/.repos/${ROS_DISTRO}

vcs import $@ --input $CORE_REPOS_DIR/main.repos --recursive $CORE_DIR;
if [ -f $CORE_REPOS_DIR/extra.repos ]; then
    vcs import $@ --input $CORE_REPOS_DIR/extra.repos --recursive $CORE_DIR;
fi
if [ -d $CORE_REPOS_DIR/patches ]; then
    for patch_script in $(find $CORE_REPOS_DIR/patches -name "*.sh"); do
        patch_script_abspath=$(realpath $patch_script);
        patch_script_relpath=$(realpath --relative-to=$CORE_REPOS_DIR/patches/ $patch_script);
        patch_target=$CORE_DIR/${patch_script_relpath%.sh};
        pushd $patch_target;
        echo "Patching $patch_target";
        /bin/bash $patch_script_abspath;
        popd;
    done;
fi
if [ -f $CORE_REPOS_DIR/patched.repos ]; then
    vcs import $@ --force --input $CORE_REPOS_DIR/patched.repos --recursive $CORE_DIR;
fi
