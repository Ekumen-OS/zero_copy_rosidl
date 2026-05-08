#!/bin/bash

# Copyright 2026 Ekumenlabs Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -o errexit

cd $(dirname "$(readlink -f "$0")")

[[ ! -z "${WITHIN_DEV}" ]] && echo "Already in the development environment!" && exit 1

# function to print help options
function print_help {
    echo "Usage: $(basename $0) [-b|--build] [-s|--service SERVICE_NAME] [-h|--help] [-- COMMAND [ARGS...]]"
    echo "  --build                      Build the image before starting the container."
    echo "  --service                    Set the service name (default: unpatched_workspace). Options: unpatched_workspace, patched_workspace"
    echo "  --help                       Display this help message."
    echo "  --                           Everything after this is used as the command to execute in the container."
}

set +o errexit
VALID_ARGS=$(OPTERR=1 getopt -o bhdas: --long build,help,service: -- "$@")
RET_CODE=$?
set -o errexit

if [[ $RET_CODE -eq 1 ]]; then
    print_help
    exit 1;
fi
if [[ $RET_CODE -ne 0 ]]; then
    >&2 echo "Unexpected getopt error"
    exit 1;
fi

BUILD=false
SERVICE_NAME="unpatched_workspace"

eval set -- "$VALID_ARGS"
while [[ "$1" != "" ]]; do
    case "$1" in
    -b | --build)
        BUILD=true
        shift
        ;;
    -s | --service)
        SERVICE_NAME="$2"
        shift 2
        ;;
    -h | --help)
        print_help
        exit 0
        ;;
    --) # start of positional arguments
        shift
        break
        ;;
    *)
        >&2 echo "Unrecognized positional argument: $1"
        print_help
        exit 1
        ;;
    esac
done

# Capture any remaining arguments after -- as the command to execute
COMMAND_ARGS=("$@")

CONTAINER_NAME="${USER}_${SERVICE_NAME}"

if [[ "$BUILD" = true ]]; then
 USERID=$(id -u) GROUPID=$(id -g) docker compose build ${SERVICE_NAME} \
 && echo "Built ${SERVICE_NAME} image." || \
    { echo "Failed to build ${SERVICE_NAME} image."; exit 1; }
fi

USERID=$(id -u) GROUPID=$(id -g) \
    docker compose run \
    --name ${CONTAINER_NAME} \
    --remove-orphans \
    --rm ${SERVICE_NAME} "${COMMAND_ARGS[@]}"
