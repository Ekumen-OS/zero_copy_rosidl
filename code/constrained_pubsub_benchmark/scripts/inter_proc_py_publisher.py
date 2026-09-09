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
Cross-process benchmark publisher (Python side).

Publishes Image messages on the configured topic at a fixed rate for a
fixed duration, either as a copy baseline (standard Image, non-loaned) or
as a constrained loaned publisher (experimental Image). Emits the raw CSV
header plus one ``publish`` event row per sent message on stdout;
diagnostics go to stderr. Intended to be paired with a subscriber process
(C++ or Python) and orchestrated by inter_proc_benchmark.py.
"""

import sys
import time

from benchmark_common import build_parser  # noqa: E402
from benchmark_common import csv_header  # noqa: E402
from benchmark_common import encode_sample_metadata  # noqa: E402
from benchmark_common import Event  # noqa: E402
from benchmark_common import fill_image  # noqa: E402
from benchmark_common import make_image_constraints  # noqa: E402
from benchmark_common import mono_now_ns  # noqa: E402
from benchmark_common import parse_payload  # noqa: E402
from benchmark_common import resolve_config  # noqa: E402
from benchmark_common import RunContext  # noqa: E402

import rclpy  # noqa: E402
from rclpy.executors import SingleThreadedExecutor  # noqa: E402
from rclpy.qos import HistoryPolicy  # noqa: E402
from rclpy.qos import QoSProfile, ReliabilityPolicy  # noqa: E402

from sensor_msgs.msg import Image  # noqa: E402
from sensor_msgs.msg.experimental import Image as ExperimentalImage  # noqa: E402

from transport_config import apply_backend_env  # noqa: E402


def main():
    """Run the fixed-rate raw publish loop, emitting CSV rows on stdout."""
    args = build_parser().parse_args()
    payload_bytes = parse_payload(args.payload_bytes)
    message, config, backend = resolve_config(args)
    apply_backend_env(backend)

    rclpy.init()
    node = rclpy.create_node('inter_proc_py_publisher')
    # One executor for the whole run; rclpy.spin_once would build one
    # per call.
    executor = SingleThreadedExecutor()
    executor.add_node(node)

    qos = QoSProfile(
        depth=args.qos_depth,
        history=HistoryPolicy.KEEP_LAST,
        reliability=ReliabilityPolicy.BEST_EFFORT
        if args.reliability == 'best_effort' else ReliabilityPolicy.RELIABLE,
    )

    # Buffer every row; a single write at the end keeps stdout I/O out of
    # the publish loop.
    ctx = RunContext.from_args(
        args, process='pub', direction_fallback='', backend=backend,
        expected_samples=args.duration_sec * args.publish_rate_hz,
        payload_bytes=payload_bytes)

    print(csv_header(), flush=True)

    sent = 0
    # Everything from publisher creation on is guarded: endpoint creation
    # can fail loudly (e.g. data sharing ON with an unbounded type), and
    # that must surface as an error row, never as a traceback.
    try:
        if config == 'copy':
            msg_type = Image if message == 'std' else ExperimentalImage
            pub = node.create_publisher(msg_type, args.topic, qos)
            use_loan = False
            # Fill once: geometry and payload are run-constant; only the
            # per-sample stamp/frame_id change inside the loop.
            msg = msg_type()
            fill_image(msg, payload_bytes, 0, args.fill_encoding,
                       args.fill_frame_id, args.fill_data_ratio)
        else:
            constraints = make_image_constraints(
                payload_bytes, strict=args.strict in ('true', '1'))
            pub = node.create_publisher(
                ExperimentalImage, args.topic, qos, constraints=constraints)
            use_loan = True

        # Wait for a matching subscriber before publishing: in a two-process
        # setup DDS discovery takes longer than the first tick.
        deadline = time.monotonic() + 10
        while pub.get_subscription_count() == 0 and time.monotonic() < deadline:
            executor.spin_once(timeout_sec=0.02)
        print('DEBUG wait_for_match: sub_count=%d' % pub.get_subscription_count(),
              file=sys.stderr, flush=True)
        print('PUB_READY', file=sys.stderr, flush=True)
        # Barrier: the orchestrator releases the publish loop only after
        # the subscriber signals its window is open (see inter_proc_cpp_publisher).
        if not sys.stdin.readline():
            raise RuntimeError('stdin closed before go-signal')

        period = 1.0 / args.publish_rate_hz
        t_start = time.monotonic()
        t_end = t_start + args.duration_sec
        next_publish = t_start
        # Deterministic deadline dither (uniform +/- jitter fraction, fixed
        # seed): breaks rigid-grid beating against fixed-period middleware
        # timers while keeping the mean rate exact. 0.0 = exact metronome.
        import random
        jitter_rng = random.Random(args.publish_jitter_seed)
        jitter_frac = args.publish_jitter
        if not 0.0 <= jitter_frac < 1.0:
            raise ValueError('--publish-jitter must be in [0.0, 1.0)')
        while time.monotonic() < t_end:
            if use_loan:
                with pub.borrow_loaned_message() as loan:
                    fill_image(loan, payload_bytes, sent,
                               args.fill_encoding, args.fill_frame_id,
                               args.fill_data_ratio)
                    send_ns = mono_now_ns()
                    encode_sample_metadata(loan, sent, send_ns)
                    pub.publish_loaned_message(loan)
            else:
                send_ns = mono_now_ns()
                encode_sample_metadata(msg, sent, send_ns)
                pub.publish(msg)
            ctx.emit(Event.PUBLISH, sent, send_ns, send_ns, -1, -1)
            sent += 1
            next_publish += period
            if jitter_frac > 0.0:
                next_publish += period * jitter_rng.uniform(
                    -jitter_frac, jitter_frac)
            now = time.monotonic()
            if next_publish < now:
                next_publish = now  # Do not burst if we fell behind.
            time.sleep(max(0.0, next_publish - now))
            executor.spin_once(timeout_sec=0.0)
        # Single write of every buffered row, after the publish loop.
        ctx.flush_rows()
    except Exception as exc:
        ctx.emit(Event.ERROR, 0, -1, -1, -1, -1, error=str(exc))
        ctx.flush_rows()
        print('publisher error: %s' % exc, file=sys.stderr, flush=True)
        rclpy.shutdown()
        return 1

    print('DONE sent=%d run_id=%s' % (sent, ctx.run_id),
          file=sys.stderr, flush=True)
    rclpy.shutdown()
    return 0


if __name__ == '__main__':
    sys.exit(main())
