#!/usr/bin/env python3
# Copyright 2026 Ekumen Inc.
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

"""
Run benchmark steps and merge their raw CSV event streams.

Single-run mode runs one publisher/subscriber pair (or one intra-process
runner) and merges its raw CSV stream. Sweep mode (--sweep) iterates the
payload x frequency x transport grid, running one step at a time with a
unique topic and run id per step. Output is always raw CSV: the header
once, then every data row. No aggregates, no plots; post-processing
derives all statistics.

Usage:
    inter_proc_benchmark.py --direction cpp_to_cpp|cpp_to_py|py_to_cpp|py_to_py
        [benchmark options]
    inter_proc_benchmark.py --sweep --direction <any of the six>
        [--sweep-payloads ...] [--sweep-freqs ...] [--sweep-shm ...]
        [--duration-per-step ...] [benchmark options]
"""

import csv
import os
import subprocess
import sys
import tempfile
import threading
import time

from benchmark_common import build_parser  # noqa: E402
from benchmark_common import COLUMNS  # noqa: E402
from benchmark_common import csv_header  # noqa: E402
from benchmark_common import default_frequency_grid  # noqa: E402
from benchmark_common import format_sample  # noqa: E402
from benchmark_common import make_run_id  # noqa: E402
from benchmark_common import new_sample  # noqa: E402
from benchmark_common import parse_frequency_grid  # noqa: E402
from benchmark_common import parse_payload  # noqa: E402
from benchmark_common import parse_payload_grid  # noqa: E402
from benchmark_common import resolve_config  # noqa: E402
from transport_config import BACKEND_ENV_VAR  # noqa: E402
from transport_config import package_profiles_dir  # noqa: E402
from transport_config import PROFILES_FILE_ENV_VAR  # noqa: E402
from transport_config import render_topic_profile  # noqa: E402
from transport_config import transport_env  # noqa: E402
from transport_config import transport_profile_filename  # noqa: E402

#: Explicit transports allowed in a sweep (auto would be meaningless).
SWEEP_TRANSPORTS = ('udp', 'shmem', 'shmem_ds')

#: Seconds a subscriber may take to become ready.
READY_TIMEOUT_SEC = 30.0

#: Seconds both ends may take to reach the discovery barrier
#: (PUB_READY + SUB_MATCHED) before the publisher is released.
BARRIER_TIMEOUT_SEC = 30.0

#: Settle time after READY so DDS discovery completes before publishing.
SETTLE_SEC = 1.0

#: Extra wall time per step beyond duration + grace (startup, discovery).
STEP_MARGIN_SEC = 60.0


class LineCollector:
    """
    Collect lines from a subprocess stream into a list.

    When a log file is provided, each line is also appended to it (tee), so
    process output is preserved on disk for diagnostics.
    """

    def __init__(self, proc, stream='stdout', log_file=None):
        """Start the background reader thread for the given stream."""
        self._proc = proc
        self._stream = getattr(proc, stream)
        self._log_file = log_file
        self.lines = []
        self._thread = threading.Thread(target=self._read, daemon=True)
        self._thread.start()

    def _read(self):
        for line in self._stream:
            self.lines.append(line)
            if self._log_file is not None:
                self._log_file.write(line)
                self._log_file.flush()

    def wait_for(self, marker, timeout_sec):
        """Wait until a stripped line equals marker; False on timeout."""
        deadline = time.monotonic() + timeout_sec
        while time.monotonic() < deadline:
            for line in self.lines:
                if line.strip() == marker:
                    return True
            time.sleep(0.02)
        return False


def data_rows(*line_lists):
    """Yield non-empty, non-header CSV rows from collected line lists."""
    for lines in line_lists:
        for line in lines:
            stripped = line.strip()
            if not stripped or stripped.startswith('run_id,'):
                continue
            yield stripped


def checked_rows(rows):
    """
    Yield well-formed rows, warning about malformed stdout lines.

    Middleware libraries occasionally log to stdout; such lines must never
    corrupt the CSV stream. Raw per-stream logs under --prof-dir keep them.
    """
    dropped = 0
    for row in rows:
        if len(next(csv.reader([row]))) != len(COLUMNS):
            dropped += 1
            if dropped <= 3:
                sys.stderr.write('dropping malformed row: %s\n' % row[:160])
            continue
        yield row
    if dropped:
        sys.stderr.write('dropped %d malformed rows\n' % dropped)


def orchestrator_error_row(run_id, direction, config, message, backend,
                           transport, payload_bytes, frequency_hz,
                           step_index, process, error):
    """Build an error row recording a failed orchestration step."""
    return format_sample(new_sample(
        run_id=run_id, event='error', process=process, direction=direction,
        config=config, message=message, backend=backend, transport=transport,
        payload_bytes=payload_bytes, target_frequency_hz=frequency_hz,
        step_index=step_index, status='error', error=error))


def child_env(base_env, backend, transport, profile_file=None,
              profiles_dir=None):
    """Apply backend and transport selection to a child environment."""
    env = dict(base_env)
    if backend == 'xcdr':
        env[BACKEND_ENV_VAR] = 'xcdr'
    else:
        env.pop(BACKEND_ENV_VAR, None)
    for key, value in transport_env(transport, profiles_dir).items():
        if value is None:
            env.pop(key, None)
        else:
            env[key] = value
    if profile_file is not None:
        env[PROFILES_FILE_ENV_VAR] = profile_file
    return env


def write_profile_file(content, prof_dir, name):
    """Write rendered profile content, returning (path, temporary)."""
    if prof_dir is not None:
        os.makedirs(prof_dir, exist_ok=True)
        path = os.path.join(prof_dir, name)
        with open(path, 'w') as handle:
            handle.write(content)
        return path, False
    fd, path = tempfile.mkstemp(prefix='ds_profile_', suffix='.xml')
    with os.fdopen(fd, 'w') as handle:
        handle.write(content)
    return path, True


def prepare_step_profile(transport, topic, prof_dir):
    """
    Render role-specific topic-scoped profiles for shmem_ds.

    Returns (pub_file, sub_file, temporary): the publishing process uses
    pub_file (data-sharing capable writer); the receiving process and
    intra-process runners use sub_file (dynamic receiving end). Other
    transports return (None, None, False). Temporary files (no prof_dir)
    must be deleted by the caller after the step; files under prof_dir
    are kept for diagnostics.
    """
    if transport != 'shmem_ds':
        return None, None, False
    base = os.path.join(
        package_profiles_dir(), transport_profile_filename(transport))
    pub_file, pub_temp = write_profile_file(
        render_topic_profile(base, topic, 'pub'), prof_dir,
        'ds_profile_pub.xml')
    sub_file, sub_temp = write_profile_file(
        render_topic_profile(base, topic, 'sub'), prof_dir,
        'ds_profile_sub.xml')
    return pub_file, sub_file, pub_temp or sub_temp


def cleanup_step_profile(pub_file, sub_file, temporary):
    """Delete temporary rendered profiles, ignoring missing files."""
    if not temporary:
        return
    for path in (pub_file, sub_file):
        if path is None:
            continue
        try:
            os.unlink(path)
        except OSError:
            pass


def open_logs(prof_dir, *names):
    """Open per-stream log files under prof_dir, or Nones without it."""
    if prof_dir is None:
        return [None] * len(names)
    os.makedirs(prof_dir, exist_ok=True)
    return [open(os.path.join(prof_dir, name), 'w') for name in names]


def close_logs(files):
    """Close log files, skipping Nones."""
    for handle in files:
        if handle is not None:
            handle.close()


def run_inter_pair(
        pub_cmd, sub_cmd, pub_env, sub_env, prof_dir, timeout_sec):
    """
    Run one publisher/subscriber pair; return (ok, rows, process, error).

    Rows holds merged data rows (possibly partial on failure); process and
    error describe the failure for the orchestrator error row. Each side
    runs under its own environment (role-specific XML profiles). The
    publisher is released via stdin only after both ends report mutual
    discovery (PUB_READY + SUB_MATCHED), so no sample predates the
    receive window.
    """
    sub_err_log, pub_err_log, sub_out_log, pub_out_log = open_logs(
        prof_dir, 'sub_stderr.log', 'pub_stderr.log',
        'sub_stdout.log', 'pub_stdout.log')
    try:
        sub_proc = subprocess.Popen(
            sub_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, env=sub_env)
        sub_out = LineCollector(sub_proc, 'stdout', sub_out_log)
        sub_err = LineCollector(sub_proc, 'stderr', sub_err_log)
        if not sub_err.wait_for('READY', timeout_sec=READY_TIMEOUT_SEC):
            sub_proc.kill()
            rows = list(data_rows(sub_out.lines))
            return False, rows, 'sub', 'subscriber did not become ready'
        time.sleep(SETTLE_SEC)

        pub_proc = subprocess.Popen(
            pub_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            stdin=subprocess.PIPE, text=True, env=pub_env)
        pub_out = LineCollector(pub_proc, 'stdout', pub_out_log)
        pub_err = LineCollector(pub_proc, 'stderr', pub_err_log)

        if not pub_err.wait_for('PUB_READY', timeout_sec=BARRIER_TIMEOUT_SEC):
            pub_proc.kill()
            sub_proc.kill()
            rows = list(data_rows(pub_out.lines, sub_out.lines))
            return False, rows, 'pub', 'publisher did not match in time'
        if not sub_err.wait_for('SUB_MATCHED', timeout_sec=BARRIER_TIMEOUT_SEC):
            pub_proc.kill()
            sub_proc.kill()
            rows = list(data_rows(pub_out.lines, sub_out.lines))
            return False, rows, 'sub', 'subscriber did not match in time'
        try:
            pub_proc.stdin.write('go\n')
            pub_proc.stdin.flush()
            pub_proc.stdin.close()
        except (BrokenPipeError, ValueError):
            pub_proc.kill()
            sub_proc.kill()
            rows = list(data_rows(pub_out.lines, sub_out.lines))
            return False, rows, 'pub', 'publisher exited before release'

        try:
            pub_rc = pub_proc.wait(timeout=timeout_sec)
            sub_rc = sub_proc.wait(timeout=timeout_sec)
        except subprocess.TimeoutExpired:
            pub_proc.kill()
            sub_proc.kill()
            rows = list(data_rows(pub_out.lines, sub_out.lines))
            return False, rows, 'orchestrator', 'step timed out'
        rows = list(data_rows(pub_out.lines, sub_out.lines))
        if pub_rc != 0 or sub_rc != 0:
            return False, rows, 'orchestrator', \
                'benchmark processes failed (pub=%d, sub=%d)' % (pub_rc, sub_rc)
        return True, rows, '', ''
    finally:
        close_logs([sub_err_log, pub_err_log, sub_out_log, pub_out_log])


def run_single_process(cmd, env, prof_dir, timeout_sec):
    """Run one intra-process benchmark; return (ok, rows, process, error)."""
    (out_log,) = open_logs(prof_dir, 'stdout.log')
    try:
        proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, env=env)
        out = LineCollector(proc, 'stdout', out_log)
        try:
            rc = proc.wait(timeout=timeout_sec)
        except subprocess.TimeoutExpired:
            proc.kill()
            return False, list(data_rows(out.lines)), \
                'orchestrator', 'step timed out'
        if rc != 0:
            return False, list(data_rows(out.lines)), \
                'orchestrator', 'benchmark process failed (rc=%d)' % rc
        return True, list(data_rows(out.lines)), '', ''
    finally:
        close_logs([out_log])


def step_commands(paths, direction, common):
    """Build the child command(s) for one step; intra returns a single cmd."""
    cpp_pub = paths['cpp_pub']
    cpp_sub = paths['cpp_sub']
    py_pub = paths['py_pub']
    py_sub = paths['py_sub']
    if direction == 'cpp_to_py':
        return ([cpp_pub] + common, [sys.executable, '-O', py_sub] + common)
    if direction == 'py_to_cpp':
        return ([sys.executable, '-O', py_pub] + common, [cpp_sub] + common)
    if direction == 'cpp_to_cpp':
        return ([cpp_pub] + common, [cpp_sub] + common)
    if direction == 'py_to_py':
        return ([sys.executable, py_pub] + common, [sys.executable, py_sub] + common)
    if direction == 'intra_cpp':
        return ([paths['intra_cpp']] + common,)
    return ([sys.executable, paths['intra_py']] + common,)


def step_common(args, run_id, payload_bytes, frequency_hz, transport,
                step_index, topic, duration_sec):
    """Build the shared child argument list for one step."""
    # Note: the C++ executables parse "--flag value" (space-separated), so
    # the shared argument list uses that form (Python argparse accepts both).
    common = [
        '--message', args.message,
        '--config', args.config,
        '--strict', args.strict,
        '--payload-bytes', str(payload_bytes),
        '--fill-encoding', str(args.fill_encoding),
        '--fill-frame-id', str(args.fill_frame_id),
        '--fill-data-ratio', str(args.fill_data_ratio),
        '--qos-depth', str(args.qos_depth),
        '--reliability', args.reliability,
        '--publish-rate-hz', repr(frequency_hz),
        '--publish-jitter', repr(args.publish_jitter),
        '--publish-jitter-seed', str(args.publish_jitter_seed),
        '--duration-sec', str(duration_sec),
        '--grace-sec', str(args.grace_sec),
        '--backend', args.backend,
        '--transport', transport,
        '--run-id', run_id,
        '--direction', args.direction,
        '--step-index', str(step_index),
        '--topic', topic,
    ]
    return common


def run_single(args, paths):
    """Run one benchmark step and merge its CSV stream."""
    message, config, backend = resolve_config(args)
    run_id = args.run_id or make_run_id(
        message, config, backend, args.direction,
        parse_payload(args.payload_bytes), args.publish_rate_hz,
        args.transport, args.reliability)
    topic = f'/inter_proc_benchmark_{os.getpid()}'
    common = step_common(
        args, run_id, parse_payload(args.payload_bytes),
        args.publish_rate_hz, args.transport, args.step_index,
        topic, args.duration_sec)
    pub_file, sub_file, temp_profile = prepare_step_profile(
        args.transport, topic, args.prof_dir)
    pub_env = child_env(os.environ, backend, args.transport, pub_file)
    sub_env = child_env(os.environ, backend, args.transport, sub_file)
    timeout_sec = args.duration_sec + args.grace_sec + STEP_MARGIN_SEC

    print(csv_header(), flush=True)
    try:
        if args.direction in ('intra_cpp', 'intra_py'):
            (cmd,) = step_commands(paths, args.direction, common)
            ok, rows, process, error = run_single_process(
                cmd, sub_env, args.prof_dir, timeout_sec)
        else:
            pub_cmd, sub_cmd = step_commands(paths, args.direction, common)
            ok, rows, process, error = run_inter_pair(
                pub_cmd, sub_cmd, pub_env, sub_env, args.prof_dir,
                timeout_sec)
        for row in checked_rows(rows):
            print(row, flush=True)
    finally:
        cleanup_step_profile(pub_file, sub_file, temp_profile)
    if not ok:
        print(orchestrator_error_row(
            run_id, args.direction, config, message, backend,
            args.transport, parse_payload(args.payload_bytes),
            args.publish_rate_hz, args.step_index,
            process or 'orchestrator', error), flush=True)
        sys.stderr.write('benchmark step failed: %s\n' % error)
        return 1
    return 0


def parse_sweep_grids(args):
    """Parse and validate sweep payload/frequency/transport grids."""
    payloads = parse_payload_grid(args.sweep_payloads)
    if args.sweep_freqs is not None:
        freqs = parse_frequency_grid(args.sweep_freqs)
    else:
        freqs = default_frequency_grid()
    transports = [item.strip() for item in args.sweep_shm.split(',')]
    for transport in transports:
        if transport not in SWEEP_TRANSPORTS:
            raise ValueError(
                '--sweep-shm must list udp,shmem,shmem_ds (got %r)' % transport)
    return payloads, freqs, transports


def run_sweep(args, paths, payloads, freqs, transports):
    """Iterate the payload x frequency x transport grid, one step at a time."""
    message, config, backend = resolve_config(args)
    steps = [(p, f, t) for p in payloads for f in freqs for t in transports]

    print(csv_header(), flush=True)
    failures = 0
    for step_index, (payload_bytes, frequency_hz, transport) in enumerate(steps):
        run_id = make_run_id(
            message, config, backend, args.direction, payload_bytes,
            frequency_hz, transport, args.reliability)
        sys.stderr.write(
            '[%d/%d] %s\n' % (step_index + 1, len(steps), run_id))
        sys.stderr.flush()
        topic = f'/benchmark_{os.getpid()}_{step_index}'
        common = step_common(
            args, run_id, payload_bytes, frequency_hz, transport,
            step_index, topic, args.duration_per_step)
        prof_dir = None
        if args.prof_dir is not None:
            prof_dir = os.path.join(args.prof_dir, 'step_%04d' % step_index)
        pub_file, sub_file, temp_profile = prepare_step_profile(
            transport, topic, prof_dir)
        pub_env = child_env(os.environ, backend, transport, pub_file)
        sub_env = child_env(os.environ, backend, transport, sub_file)
        timeout_sec = args.duration_per_step + args.grace_sec + STEP_MARGIN_SEC

        try:
            if args.direction in ('intra_cpp', 'intra_py'):
                (cmd,) = step_commands(paths, args.direction, common)
                ok, rows, process, error = run_single_process(
                    cmd, sub_env, prof_dir, timeout_sec)
            else:
                pub_cmd, sub_cmd = step_commands(paths, args.direction, common)
                ok, rows, process, error = run_inter_pair(
                    pub_cmd, sub_cmd, pub_env, sub_env, prof_dir,
                    timeout_sec)
            for row in checked_rows(rows):
                print(row, flush=True)
        finally:
            cleanup_step_profile(pub_file, sub_file, temp_profile)
        if not ok:
            print(orchestrator_error_row(
                run_id, args.direction, config, message, backend,
                transport, payload_bytes, frequency_hz, step_index,
                process or 'orchestrator', error), flush=True)
            sys.stderr.write('step failed: %s\n' % error)
            sys.stderr.flush()
            failures += 1
    sys.stderr.write(
        'sweep done: %d steps, %d failures\n' % (len(steps), failures))
    return 1 if failures else 0


def main():
    """Run a single benchmark step or a full stepped sweep."""
    parser = build_parser()
    parser.add_argument(
        '--sweep',
        action='store_true',
        default=False,
        help='Iterate the payload x frequency x transport grid.')
    parser.add_argument(
        '--prof-dir',
        default=None,
        help='Directory to save subprocess stdout and stderr to; '
             'default discards stderr and keeps stdout in memory only.')
    args = parser.parse_args()

    here = os.path.dirname(os.path.realpath(__file__))
    paths = {
        'cpp_pub': os.path.join(here, 'inter_proc_cpp_publisher'),
        'cpp_sub': os.path.join(here, 'inter_proc_cpp_subscriber'),
        'py_pub': os.path.join(here, 'inter_proc_py_publisher.py'),
        'py_sub': os.path.join(here, 'inter_proc_py_subscriber.py'),
        'intra_cpp': os.path.join(here, 'intra_cpp_benchmark'),
        'intra_py': os.path.join(here, 'intra_proc_benchmark.py'),
    }

    if args.sweep:
        if args.direction not in (
                'cpp_to_cpp', 'cpp_to_py', 'py_to_cpp', 'py_to_py',
                'intra_cpp', 'intra_py'):
            parser.error('--direction is required for --sweep')
        try:
            payloads, freqs, transports = parse_sweep_grids(args)
        except ValueError as exc:
            parser.error(str(exc))
        return run_sweep(args, paths, payloads, freqs, transports)
    if args.direction not in (
            'cpp_to_cpp', 'cpp_to_py', 'py_to_cpp', 'py_to_py',
            'intra_cpp', 'intra_py'):
        parser.error(
            '--direction is required: '
            'cpp_to_cpp|cpp_to_py|py_to_cpp|py_to_py|intra_cpp|intra_py')
    return run_single(args, paths)


if __name__ == '__main__':
    sys.exit(main())
