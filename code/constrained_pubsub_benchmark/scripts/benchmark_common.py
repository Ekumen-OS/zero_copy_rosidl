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

"""Shared helpers for the cross-process (C++ <-> Python) pub/sub benchmark."""

import argparse
import enum
import math
import os
import sys
import time

import numpy as np


#: Benchmark case axes. Message (--message) selects the ROS message type;
#: config (--config) selects the data path (plain copy vs constrained
#: loaned publisher, optionally with a constrained subscription); backend
#: (--backend) selects the rmw_fastrtps serialization backend. FastCDR is
#: only valid with the copy path; constrained configs require --message exp.
MESSAGES = ('std', 'exp')

CONFIGS = ('copy', 'constrained_pub_only', 'constrained_pub_sub')

#: DDS transport profile ids.
TRANSPORTS = ('auto', 'udp', 'shmem', 'shmem_ds')

#: Serialization backend ids.
BACKENDS = ('auto', 'fastcdr', 'xcdr')

#: Direction labels used by the orchestrator and intra-process runners.
DIRECTIONS = (
    'cpp_to_cpp', 'cpp_to_py', 'py_to_cpp', 'py_to_py',
    'intra_cpp', 'intra_py',
)

#: Default stepped sweep grids.
DEFAULT_PAYLOAD_GRID = '4,40,400,4K,40K,400K,4M,40M'
DEFAULT_TRANSPORT_GRID = 'udp,shmem,shmem_ds'


def build_parser():
    """Build the argument parser shared by publisher, subscriber and launcher."""
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        '--message', choices=MESSAGES, default='std',
        help='ROS message type on the topic.')
    parser.add_argument(
        '--config', choices=CONFIGS, default='copy',
        help='Data path: plain copy vs constrained loaned pub/sub.')
    parser.add_argument('--backend', choices=BACKENDS, default='auto')
    parser.add_argument('--transport', choices=TRANSPORTS, default='auto')
    parser.add_argument('--run-id', default='')
    parser.add_argument('--direction', choices=DIRECTIONS, default=None)
    parser.add_argument('--step-index', type=int, default=-1)
    parser.add_argument('--strict', choices=['true', 'false', '1', '0'], default='false')
    parser.add_argument('--payload-bytes', default='1000')
    parser.add_argument('--fill-encoding', type=int, default=0)
    parser.add_argument('--fill-frame-id', type=int, default=0)
    parser.add_argument('--fill-data-ratio', type=float, default=0.0)
    parser.add_argument('--qos-depth', type=int, default=10)
    parser.add_argument('--reliability', choices=['reliable', 'best_effort'], default='reliable')
    parser.add_argument('--publish-rate-hz', type=float, default=100.0)
    parser.add_argument('--publish-jitter', type=float, default=0.0)
    parser.add_argument('--publish-jitter-seed', type=int, default=42)
    parser.add_argument('--duration-sec', type=float, default=30.0)
    parser.add_argument('--sweep-payloads', default=DEFAULT_PAYLOAD_GRID)
    parser.add_argument('--sweep-freqs', default=None)
    parser.add_argument('--sweep-shm', default=DEFAULT_TRANSPORT_GRID)
    parser.add_argument('--duration-per-step', type=float, default=10.0)
    parser.add_argument('--topic', default='/inter_proc_benchmark')
    parser.add_argument('--grace-sec', type=float, default=2.0)
    return parser


def validate_case(message, config, backend):
    """Validate a case triple; raises ValueError on violation."""
    if config != 'copy' and message != 'exp':
        raise ValueError('constrained configs require --message exp')
    resolve_backend(message, config, backend)


def resolve_config(args):
    """Resolve the effective (message, config, backend) triple from args."""
    validate_case(args.message, args.config, args.backend)
    return args.message, args.config, resolve_backend(
        args.message, args.config, args.backend)


def resolve_backend(message, config, backend):
    """
    Resolve the effective backend.

    Explicit wins, else the default (FastCDR for a standard-message
    copy, XCDR everywhere else).
    """
    if backend != 'auto':
        if backend == 'fastcdr' and config != 'copy':
            raise ValueError('fastcdr backend requires the copy config')
        return backend
    if config == 'copy' and message == 'std':
        return 'fastcdr'
    return 'xcdr'


def parse_payload_grid(s):
    """Parse a comma-separated payload grid (each item takes K/KB/M/MB)."""
    items = [part.strip() for part in s.split(',')]
    if not items or any(not part for part in items):
        raise ValueError('empty payload grid entry')
    return [parse_payload(part) for part in items]


def default_frequency_grid():
    """Return the default stepped grid: 10^(k/3) Hz for k = 0..9."""
    return [10.0 ** (k / 3.0) for k in range(10)]


def parse_frequency_grid(s):
    """Parse a comma-separated frequency grid in Hz (all values > 0)."""
    items = [part.strip() for part in s.split(',')]
    if not items or any(not part for part in items):
        raise ValueError('empty frequency grid entry')
    freqs = [float(part) for part in items]
    if any(not (hz > 0.0) for hz in freqs):
        raise ValueError('frequency must be positive')
    return freqs


def format_frequency_hz(hz):
    """Format a frequency for run ids ('10Hz', '2p15443Hz'). Deterministic."""
    return ('%.6g' % hz).replace('.', 'p') + 'Hz'


def make_run_id(message, config, backend, direction, payload_bytes,
                frequency_hz, transport, reliability='reliable'):
    """
    Build a run id from its coordinates.

    E.g. 'exp_constrained_pub_sub_xcdr__cpp_to_cpp__...__shmem__reliable'.
    Callers pass the resolved backend. Reliability defaults to
    'reliable' so historical ids without the coordinate keep parsing.
    """
    direction = direction or 'manual'
    return '%s_%s_%s__%s__%dB__%s__%s__%s' % (
        message, config, backend, direction, payload_bytes,
        format_frequency_hz(frequency_hz), transport, reliability)


def auto_run_id(message, config, backend, direction, payload_bytes,
                frequency_hz, transport, reliability='reliable'):
    """Build a run id with the process id appended (manual runs only)."""
    return '%s__pid%d' % (
        make_run_id(message, config, backend, direction, payload_bytes,
                    frequency_hz, transport, reliability),
        os.getpid())


def parse_payload(s):
    """Parse a payload size, accepting K/KB/M/MB suffixes (case-insensitive)."""
    upper = s.upper().strip()
    multiplier = 1
    num = upper
    if upper.endswith('MB'):
        multiplier = 1000000
        num = upper[:-2]
    elif upper.endswith('M'):
        multiplier = 1000000
        num = upper[:-1]
    elif upper.endswith('KB'):
        multiplier = 1000
        num = upper[:-2]
    elif upper.endswith('K'):
        multiplier = 1000
        num = upper[:-1]
    return int(num) * multiplier


def fill_image(msg, payload_bytes, seq, fill_encoding, fill_frame_id, fill_data_ratio):
    """Fill an Image message (standard or experimental) with the given shape."""
    msg.header.stamp.sec = (seq >> 32) & 0xFFFFFFFF
    msg.header.stamp.nanosec = seq & 0xFFFFFFFF
    if fill_frame_id > 0:
        msg.header.frame_id = 'f' * fill_frame_id
    else:
        msg.header.frame_id = 'benchmark'
    # NOTE: assign int, not bool: experimental bindings store is_bigendian
    # as uint8 and their strict coercion rejects Python bool for it.
    msg.is_bigendian = 0
    if fill_encoding > 0:
        msg.encoding = 'x' * fill_encoding
    else:
        msg.encoding = 'rgb8'

    if fill_data_ratio > 0.0:
        want = max(1, int(payload_bytes * fill_data_ratio))
        h = max(1, int(math.sqrt(want / 3.0)))
        w = max(1, math.ceil(want / (3.0 * h)))
        msg.height = h
        msg.width = w
        msg.step = w * 3
        try:
            msg.data.resize(want, 0xFE)
        except Exception:
            msg.data = b'\xfe' * want
    else:
        dim = max(1, int(math.sqrt(payload_bytes / 3)))
        actual = dim * dim * 3
        msg.height = dim
        msg.width = dim
        msg.step = dim * 3
        try:
            msg.data.resize(actual, 0xFE)
        except Exception:
            msg.data = b'\xfe' * actual


#: Canonical CSV column order. Stable: post-processing parses by position.
COLUMNS = (
    'run_id', 'event', 'process', 'direction', 'config', 'message',
    'backend', 'transport', 'payload_bytes', 'target_frequency_hz',
    'step_index', 'phase', 'sequence', 'observed_utc', 'observed_mono_ns',
    'send_mono_ns', 'receive_mono_ns', 'middleware_source_ns',
    'middleware_received_ns', 'status', 'error',
)

#: Largest sequence representable in the 15-digit frame_id suffix.
MAX_METADATA_SEQUENCE = 10 ** 15 - 1


def csv_header():
    """Return the canonical CSV header line."""
    return ','.join(COLUMNS)


def escape_csv_field(value):
    """Quote a CSV field containing a comma, quote, or newline."""
    text = str(value)
    if ',' not in text and '"' not in text and '\n' not in text:
        return text
    return '"%s"' % text.replace('"', '""')


def format_sample(sample):
    """Serialize a sample dict in COLUMNS order."""
    row = dict(sample)
    row['target_frequency_hz'] = '%.6g' % row['target_frequency_hz']
    return ','.join(escape_csv_field(row[column]) for column in COLUMNS)


def new_sample(**overrides):
    """Build a sample dict with sane defaults, applying overrides."""
    sample = {
        'run_id': '', 'event': 'publish', 'process': '', 'direction': '',
        'config': '', 'message': '', 'backend': '', 'transport': '',
        'payload_bytes': 0, 'target_frequency_hz': 0.0, 'step_index': -1,
        'phase': 'measured', 'sequence': 0, 'observed_utc': '',
        'observed_mono_ns': -1, 'send_mono_ns': -1, 'receive_mono_ns': -1,
        'middleware_source_ns': -1, 'middleware_received_ns': -1,
        'status': 'ok', 'error': '',
    }
    sample.update(overrides)
    return sample


class Event(enum.IntEnum):
    """Sample event type; stored as int, translated to str on flush."""

    PUBLISH = 0
    RECEIVE = 1
    ERROR = 2


#: Event value -> CSV label.
EVENT_KEYS = ('publish', 'receive', 'error')


class RunContext:
    """
    Shared run coordinates for one emitter; mirrors the C++ RunContext.

    Construct via from_args() (or explicitly with the same named fields
    as the C++ struct), then emit() per sample; emit() only buffers
    (timestamps are still captured at event time). Call flush_rows()
    once after the run to write every buffered row to stdout. Never
    write inside the hot path.

    Per-row numerics live in a pre-sized numpy int64 array
    (expected_samples with a 20% margin); per-row strings (observed UTC,
    error text) live in pre-sized lists. Phase is always "measured" and
    status derives from the event, so neither is stored. The buffer still
    grows if the estimate is exceeded, so data is never lost.
    """

    #: Numeric columns stored in the array, in order.
    NUM_COLUMNS = (
        'event', 'sequence', 'send_mono_ns', 'receive_mono_ns',
        'middleware_source_ns', 'middleware_received_ns',
    )

    def __init__(self, run_id, process, direction, config, message,
                 backend, transport, payload_bytes, target_frequency_hz,
                 step_index, expected_samples):
        """Store run constants explicitly; pre-size for expected_samples."""
        self.run_id = run_id
        self.process = process
        self.direction = direction
        self.config = config
        self.message = message
        self.backend = backend
        self.transport = transport
        self.payload_bytes = payload_bytes
        self.target_frequency_hz = target_frequency_hz
        self.step_index = step_index
        capacity = int(expected_samples * 1.2) + 8
        self._nums = np.zeros((capacity, len(self.NUM_COLUMNS)), dtype=np.int64)
        self._utc = [None] * capacity
        self._errors = [None] * capacity
        self._size = 0

    @classmethod
    def from_args(cls, args, process, direction_fallback, backend,
                  expected_samples, payload_bytes):
        """
        Build a context from parsed args.

        Explicit run id wins, otherwise minted from the resolved
        backend; explicit direction wins, otherwise the fallback.
        Manual runs omit --direction.
        """
        run_id = args.run_id or auto_run_id(
            args.message, args.config, backend,
            args.direction or direction_fallback, payload_bytes,
            args.publish_rate_hz, args.transport,
            getattr(args, 'reliability', 'reliable'))
        return cls(
            run_id=run_id, process=process,
            direction=args.direction or direction_fallback,
            config=args.config, message=args.message, backend=backend,
            transport=args.transport, payload_bytes=payload_bytes,
            target_frequency_hz=args.publish_rate_hz,
            step_index=args.step_index,
            expected_samples=expected_samples)

    def __len__(self):
        """Return the number of buffered rows."""
        return self._size

    def emit(self, event, sequence, send_mono_ns, receive_mono_ns,
             middleware_source_ns, middleware_received_ns, error=''):
        """Buffer one row; the observation UTC is captured here."""
        if self._size >= len(self._utc):
            self._grow()
        row = self._size
        self._nums[row, 0] = int(event)
        self._nums[row, 1] = sequence
        self._nums[row, 2] = send_mono_ns
        self._nums[row, 3] = receive_mono_ns
        self._nums[row, 4] = middleware_source_ns
        self._nums[row, 5] = middleware_received_ns
        self._utc[row] = utc_now_iso8601()
        self._errors[row] = error
        self._size += 1

    def _grow(self):
        """Double capacity; only runs when the margin was exceeded."""
        extra = len(self._utc)
        grown = np.zeros((extra * 2, len(self.NUM_COLUMNS)), dtype=np.int64)
        grown[:self._size] = self._nums[:self._size]
        self._nums = grown
        self._utc.extend([None] * extra)
        self._errors.extend([None] * extra)

    def flush_rows(self):
        """Write every buffered row to stdout in a single write."""
        lines = []
        for row in range(self._size):
            nums = self._nums[row]
            event = EVENT_KEYS[int(nums[0])]
            send_ns = int(nums[2])
            receive_ns = int(nums[3])
            sample = new_sample(
                run_id=self.run_id, event=event, process=self.process,
                direction=self.direction, config=self.config,
                message=self.message, backend=self.backend,
                transport=self.transport, payload_bytes=self.payload_bytes,
                target_frequency_hz=self.target_frequency_hz,
                step_index=self.step_index, sequence=int(nums[1]),
                observed_utc=self._utc[row],
                observed_mono_ns=send_ns if event == 'publish' else receive_ns,
                send_mono_ns=send_ns, receive_mono_ns=receive_ns,
                middleware_source_ns=int(nums[4]),
                middleware_received_ns=int(nums[5]),
                status='error' if event == 'error' else 'ok',
                error=self._errors[row])
            lines.append(format_sample(sample))
        if lines:
            sys.stdout.write('\n'.join(lines) + '\n')
            sys.stdout.flush()
        self._size = 0


def utc_now_iso8601():
    """Return the wall clock as ISO-8601 UTC with microsecond precision."""
    now = time.time()
    whole = int(now)
    return (
        time.strftime('%Y-%m-%dT%H:%M:%S.', time.gmtime(whole)) +
        '%06dZ' % int((now - whole) * 1e6))


def mono_now_ns():
    """Return the monotonic clock in nanoseconds."""
    return time.monotonic_ns()


def encode_sample_metadata(msg, seq, send_mono_ns):
    """
    Stamp a message with sequence and monotonic send time.

    The sequence travels in header.frame_id as 's' plus 15 zero-padded
    digits (16 chars, satisfying the c1/c2 bound); the send time travels
    in header.stamp. Call after fill_image.
    """
    if not 0 <= seq <= MAX_METADATA_SEQUENCE:
        raise ValueError('sequence out of range: %r' % (seq,))
    msg.header.frame_id = 's%015d' % seq
    msg.header.stamp.sec = send_mono_ns // 1000000000
    msg.header.stamp.nanosec = send_mono_ns % 1000000000


def decode_sample_metadata(msg):
    """Decode (sequence, send time); (None, None) for foreign traffic."""
    frame_id = msg.header.frame_id
    if not isinstance(frame_id, str):
        # Experimental bindings return a String view, not a str.
        frame_id = frame_id.as_builtin()
    if len(frame_id) != 16 or not frame_id.startswith('s'):
        return None, None
    digits = frame_id[1:]
    if not digits.isdigit():
        return None, None
    # NOTE: int() first — experimental stamps read back as fixed-width
    # scalar wrappers whose arithmetic wraps at 32 bits.
    send_mono_ns = (
        int(msg.header.stamp.sec) * 1000000000 +
        int(msg.header.stamp.nanosec))
    return int(digits), send_mono_ns


def make_image_constraints(payload_bytes, strict=False):
    """
    Build MessageConstraints for the experimental Image benchmark.

    Mirrors the C++ make_inter_proc_constraints: frame_id bound 16, encoding
    bound 64, data bound payload_bytes, blanket string limit 256.
    """
    from rosidl_runtime_cpython.constraints import MessageConstraints
    from rosidl_runtime_cpython.constraints import StringConstraint
    from rosidl_runtime_cpython.constraints import UInt8SequenceConstraint
    from sensor_msgs.msg.experimental import Image as ExperimentalImage
    type_specific = ExperimentalImage.Constraints()
    type_specific.header.frame_id = StringConstraint(size=16)
    type_specific.encoding = StringConstraint(size=64)
    type_specific.data = UInt8SequenceConstraint(size=payload_bytes)
    return MessageConstraints(
        type_specific=type_specific, max_string_length=256,
        max_total_size=0, strict=bool(strict))
