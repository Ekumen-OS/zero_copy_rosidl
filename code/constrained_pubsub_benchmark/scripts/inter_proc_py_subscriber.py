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
Cross-process benchmark subscriber (Python side).

Receives Image messages on the configured topic, either as a copy baseline
(standard Image) or with an experimental Image subscription (optionally
constrained for c2). Emits the raw CSV header plus one ``receive``
event row per message on stdout; prints READY on stderr once the
subscription exists. Intended to be paired with a publisher process
(C++ or Python) and orchestrated by inter_proc_benchmark.py.
"""

import faulthandler
import sys
import time

faulthandler.enable()

from benchmark_common import build_parser  # noqa: E402
from benchmark_common import csv_header  # noqa: E402
from benchmark_common import decode_sample_metadata  # noqa: E402
from benchmark_common import Event  # noqa: E402
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
    """Run the raw receive loop, emitting CSV rows on stdout."""
    args = build_parser().parse_args()
    payload_bytes = parse_payload(args.payload_bytes)
    message, config, backend = resolve_config(args)
    apply_backend_env(backend)

    rclpy.init()
    node = rclpy.create_node(
        'inter_proc_py_subscriber',
        start_parameter_services=False,
        enable_logger_service=False,
    )

    qos = QoSProfile(
        depth=args.qos_depth,
        history=HistoryPolicy.KEEP_LAST,
        reliability=ReliabilityPolicy.BEST_EFFORT
        if args.reliability == 'best_effort' else ReliabilityPolicy.RELIABLE,
    )

    # Buffer every row; a single write at the end keeps stdout I/O out of
    # the subscription callback. Receives never exceed publishes.
    ctx = RunContext.from_args(
        args, process='sub', direction_fallback='', backend=backend,
        expected_samples=(
            args.duration_sec + args.grace_sec) * args.publish_rate_hz,
        payload_bytes=payload_bytes)

    received = 0

    def callback(msg, info):
        nonlocal received
        recv_ns = mono_now_ns()
        seq, send_ns = decode_sample_metadata(msg)
        if seq is None:
            ctx.emit(
                Event.ERROR, 0, -1, recv_ns,
                info['source_timestamp'], info['received_timestamp'],
                error='foreign frame_id')
        else:
            ctx.emit(
                Event.RECEIVE, seq, send_ns, recv_ns,
                info['source_timestamp'], info['received_timestamp'])
            received += 1

    print(csv_header(), flush=True)

    # Everything from subscription creation on is guarded: endpoint
    # creation can fail loudly (e.g. data sharing ON with an unbounded
    # type), and that must surface as an error row, never as a traceback.
    try:
        # NOTE: keep the subscription referenced; dropping it unsubscribes.
        # The message selects the subscription type; only a constrained
        # subscriber adds constraints.
        if message == 'std':
            sub = node.create_subscription(  # noqa: F841
                Image, args.topic, callback, qos)
        elif config == 'constrained_pub_sub':
            constraints = make_image_constraints(
                payload_bytes, strict=args.strict in ('true', '1'))
            sub = node.create_subscription(  # noqa: F841
                ExperimentalImage, args.topic, callback, qos,
                constraints=constraints)
        else:
            sub = node.create_subscription(  # noqa: F841
                ExperimentalImage, args.topic, callback, qos)

        # Signal readiness to the orchestrator once the subscription exists.
        print('READY', file=sys.stderr, flush=True)

        # Single-threaded executor: callbacks fire on this thread.
        executor = SingleThreadedExecutor()
        executor.add_node(node)

        # Mutual discovery: wait for the publisher before starting the
        # receive window, otherwise the window burns while unmatched and
        # head samples are recorded as drops.
        match_start = time.monotonic()
        while (sub.get_publisher_count() == 0 and
                time.monotonic() - match_start < 30.0):
            executor.spin_once(timeout_sec=0.05)
        print('DEBUG wait_for_pub_match: pub_count=%d elapsed=%.2fs' % (
            sub.get_publisher_count(), time.monotonic() - match_start),
            file=sys.stderr, flush=True)
        if sub.get_publisher_count() == 0:
            raise RuntimeError('no matching publisher within 30s')

        # The publisher blocks on a go-signal until it sees this marker,
        # so no sample is published before the receive window opens.
        print('SUB_MATCHED', file=sys.stderr, flush=True)

        t_start = time.monotonic()
        t_end = t_start + args.duration_sec + args.grace_sec
        # 10 ms quantum (not 50 ms): longer quanta phase-lock against rigid
        # publish grids and inflate the tail; measured 50->5 ms taking
        # cpp_to_py 1M@10Hz from ~490 to ~342 mean.
        while time.monotonic() < t_end:
            executor.spin_once(timeout_sec=0.01)
        executor.shutdown()
        # Single write of every buffered row, after the receive window.
        ctx.flush_rows()
    except Exception as exc:
        ctx.emit(Event.ERROR, 0, -1, -1, -1, -1, error=str(exc))
        ctx.flush_rows()
        print('subscriber error: %s' % exc, file=sys.stderr, flush=True)
        rclpy.shutdown()
        return 1

    print('DONE received=%d run_id=%s' % (received, ctx.run_id),
          file=sys.stderr, flush=True)
    rclpy.shutdown()
    return 0


if __name__ == '__main__':
    sys.exit(main())
