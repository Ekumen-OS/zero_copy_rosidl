#!/bin/env python3

"""Import missing distro sources without changing existing checkouts."""

import os
from pathlib import Path
import subprocess
import sys

import yaml


def main():
    """Import the selected distro's manifests and patch new checkouts."""
    distro = os.environ.get('ROS_DISTRO')
    if distro not in ('jazzy', 'rolling'):
        sys.exit('Set ROS_DISTRO to jazzy or rolling.')

    for arg in sys.argv[1:]:
        option = arg.split('=', 1)[0]
        if option.startswith('--f') and '--force'.startswith(option):
            sys.exit('Force imports are not supported.')

    code_dir = Path(__file__).resolve().parent
    user_ws = os.environ.get('USER_WS')
    if user_ws and code_dir == Path(user_ws) / 'src':
        code_dir = Path.home() / 'code'
    core_dir = code_dir / 'core' / distro
    repos_dir = code_dir / 'core' / '.repos' / distro
    core_dir.mkdir(parents=True, exist_ok=True)

    patches_dir = repos_dir / 'patches'
    patches = sorted(patches_dir.rglob('*.sh'))
    targets = {
        patch: core_dir / patch.relative_to(patches_dir).with_suffix('')
        for patch in patches
    }
    existing = {
        target for target in targets.values() if os.path.lexists(target)
    }

    for name in ('patched.repos', 'main.repos', 'extra.repos'):
        manifest_path = repos_dir / name
        if not manifest_path.is_file():
            continue
        with manifest_path.open() as manifest_file:
            manifest = yaml.safe_load(manifest_file)
        repositories = manifest['repositories']
        for path in list(repositories):
            if os.path.lexists(core_dir / path):
                print(f'Skipping existing checkout: {path}', file=sys.stderr)
                del repositories[path]
        subprocess.run(
            [
                'vcs', 'import', *sys.argv[1:], '--skip-existing',
                '--input', '-', '--recursive', str(core_dir),
            ],
            input=yaml.safe_dump(manifest),
            text=True,
            check=True,
        )

    for patch, target in targets.items():
        if target in existing:
            print(f'Skipping patch for existing checkout: {target}')
            continue
        print(f'Patching {target}', flush=True)
        subprocess.run(['/bin/bash', str(patch)], cwd=target, check=True)


if __name__ == '__main__':
    main()
