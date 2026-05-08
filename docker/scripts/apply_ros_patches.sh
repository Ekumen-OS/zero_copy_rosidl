#!/bin/bash
set -e

for repo in $(find . -name ".git" -type d -prune -exec sh -c 'dirname "$@"' _ {} \;); do
    pushd "$repo" > /dev/null
    repo_name=$(basename "$repo")
    if [ -f /tmp/patches_ros/$repo_name.sh ]; then
        echo "Post-processing $repo_name repository"
        /bin/bash /tmp/patches_ros/$repo_name.sh
    fi
    popd > /dev/null
done
