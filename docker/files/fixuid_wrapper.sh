#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Missing command line argument"
    echo "Usage: $(basename "$0") <command>"
    exit 1
fi

echo
echo "Starting docker container..."
echo

exec "$@"
