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

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
cd "$SCRIPT_DIR"

function print_help {
    echo "Usage: $(basename "$0") [-b|--build] [-d|--distro DISTRO] [-s|--service SERVICE_NAME] [-h|--help] [-- COMMAND [ARGS...]]"
    echo "  --build    Build the image before starting the container."
    echo "  --distro   Set the ROS distro: jazzy (default) or rolling."
    echo "  --service  Set the Compose service (default: dev_environ)."
    echo "  --help     Display this help message."
    echo "  --         Everything after this is used as the command to execute in the container."
    echo "  In a dev container, /home/developer/code/import.py imports missing sources for ROS_DISTRO."
}

BUILD=false
DISTRO="jazzy"
SERVICE_NAME="dev_environ"

while [[ $# -gt 0 ]]; do
    case "$1" in
    -b | --build)
        BUILD=true
        shift
        ;;
    -s | --service)
        if [[ $# -lt 2 || -z "$2" ]]; then
            >&2 echo "Missing service name"
            exit 1
        fi
        SERVICE_NAME="$2"
        shift 2
        ;;
    --service=*)
        SERVICE_NAME="${1#*=}"
        shift
        ;;
    -d | --distro)
        if [[ $# -lt 2 || -z "$2" ]]; then
            >&2 echo "Missing distro"
            exit 1
        fi
        DISTRO="$2"
        shift 2
        ;;
    --distro=*)
        DISTRO="${1#*=}"
        shift
        ;;
    -h | --help)
        print_help
        exit 0
        ;;
    --)
        shift
        break
        ;;
    -*)
        >&2 echo "Unrecognized option: $1"
        print_help
        exit 1
        ;;
    *)
        break
        ;;
    esac
done

case "$DISTRO" in
    jazzy) PROJECT="docker" ;;
    rolling) PROJECT="zcr_rolling" ;;
    *)
        >&2 echo "Unsupported ROS distro: $DISTRO"
        exit 1
        ;;
esac

if [[ -z "$SERVICE_NAME" ]]; then
    >&2 echo "Missing service name"
    exit 1
fi

export USERID=$(id -u) GROUPID=$(id -g)
COMPOSE=(docker compose -p "$PROJECT" -f "$SCRIPT_DIR/$DISTRO/docker-compose.yml")

if [[ "$BUILD" = true ]]; then
    "${COMPOSE[@]}" build "$SERVICE_NAME"
    echo "Built $SERVICE_NAME image."
fi

"${COMPOSE[@]}" run --rm "$SERVICE_NAME" "$@"
