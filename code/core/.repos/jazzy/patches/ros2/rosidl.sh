#!/bin/bash
set -euo pipefail

if [[ ! -f rosidl_runtime_cpp/include/rosidl_runtime_cpp/experimental/sequence.hpp ]]; then
    echo "Skipping experimental sequence patch on baseline rosidl"
    exit 0
fi

PATCH_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PATCH=$PATCH_DIR/rosidl.sequence.patch
if git apply --reverse --check "$PATCH" 2>/dev/null; then
    echo "Experimental sequence patch is already applied"
    exit 0
fi
git apply --check "$PATCH"
git apply "$PATCH"
