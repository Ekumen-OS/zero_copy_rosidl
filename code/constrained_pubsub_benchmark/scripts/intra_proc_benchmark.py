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
Single-process benchmark (Python side).

Publishes and subscribes Image messages on one node at a fixed rate for
a fixed duration, covering all five configurations (standard and
experimental messages, FastCDR/XCDR backends, copy and constrained loaned
paths). rclpy has no intra-process bypass, so traffic always traverses
the configured DDS transport. Emits the raw CSV header plus one event row
per published/received message on stdout; diagnostics go to stderr.
"""

import concurrent.futures
import sys
import time

from benchmark_common import build_parser  # noqa: E402
from benchmark_common import csv_header  # noqa: E402
from benchmark_common import decode_sample_metadata  # noqa: E402
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
    """Run the fixed-rate intra-process loop, emitting CSV rows on stdout."""
    args = build_parser().parse_args()
    payload_bytes = parse_payload(args.payload_bytes)
    message, config, backend = resolve_config(args)
    apply_backend_env(backend)

    rclpy.init()
    # NOTE: rclpy offers no intra-process bypass; pub/sub traffic always
    # traverses the DDS transport selected for this run.
    node = rclpy.create_node(
        'intra_py_benchmark',
        start_parameter_services=False,
        enable_logger_service=False,
    )

    qos = QoSProfile(
        depth=args.qos_depth,
        history=HistoryPolicy.KEEP_LAST,
        reliability=ReliabilityPolicy.BEST_EFFORT
        if args.reliability == 'best_effort' else ReliabilityPolicy.RELIABLE,
    )

    # One buffer for publish and receive rows (post-processing joins on
    # sequence). Receives never exceed publishes.
    ctx = RunContext.from_args(
        args, process='intra', direction_fallback='intra_py',
        backend=backend,
        expected_samples=2 * args.duration_sec * args.publish_rate_hz,
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

    # Everything from endpoint creation on is guarded: endpoint creation
    # can fail loudly (e.g. data sharing ON with an unbounded type), and
    # that must surface as an error row, never as a traceback.
    try:
        if message == 'std':
            msg_type = Image
        else:
            msg_type = ExperimentalImage
        sub_constraints = None
        if config == 'constrained_pub_sub':
            sub_constraints = make_image_constraints(
                payload_bytes, strict=args.strict in ('true', '1'))

        if config == 'copy':
            pub = node.create_publisher(msg_type, args.topic, qos)
            use_loan = False
            # Fill once: geometry and payload are run-constant; only the
            # per-sample stamp/frame_id change inside the timer callback.
            pub_msg = msg_type()
            fill_image(pub_msg, payload_bytes, 0, args.fill_encoding,
                       args.fill_frame_id, args.fill_data_ratio)
        else:
            constraints = make_image_constraints(
                payload_bytes, strict=args.strict in ('true', '1'))
            pub = node.create_publisher(
                msg_type, args.topic, qos, constraints=constraints)
            use_loan = True

        # NOTE: keep the subscription referenced; dropping it unsubscribes.
        if sub_constraints is not None:
            sub = node.create_subscription(  # noqa: F841
                msg_type, args.topic, callback, qos,
                constraints=sub_constraints)
        else:
            sub = node.create_subscription(  # noqa: F841
                msg_type, args.topic, callback, qos)

        executor = SingleThreadedExecutor()
        executor.add_node(node)

        # Mutual discovery before the window: same-process matching still
        # needs a spin to complete; without it the first samples are lost.
        match_start = time.monotonic()
        while ((pub.get_subscription_count() == 0 or
                sub.get_publisher_count() == 0) and
                time.monotonic() - match_start < 30.0):
            executor.spin_once(timeout_sec=0.05)
        print('DEBUG local_match: elapsed=%.2fs' % (
            time.monotonic() - match_start), file=sys.stderr, flush=True)
        if (pub.get_subscription_count() == 0 or
                sub.get_publisher_count() == 0):
            raise RuntimeError(
                'intra-process endpoints did not match within 30s')

        # Completed by the timer when the run window elapses; lets the
        # executor stop promptly instead of polling manually.
        done = concurrent.futures.Future()
        sent = 0
        t_end = time.monotonic() + args.duration_sec

        def timer_cb():
            nonlocal sent
            if time.monotonic() >= t_end:
                if not done.done():
                    done.set_result(None)
                return
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
                encode_sample_metadata(pub_msg, sent, send_ns)
                pub.publish(pub_msg)
            ctx.emit(Event.PUBLISH, sent, send_ns, send_ns, -1, -1)
            sent += 1

        timer = node.create_timer(1.0 / args.publish_rate_hz, timer_cb)
        executor.spin_until_future_complete(
            done, timeout_sec=args.duration_sec + args.grace_sec + 5.0)
        node.destroy_timer(timer)
    except Exception as exc:
        ctx.emit(Event.ERROR, 0, -1, -1, -1, -1, error=str(exc))
        ctx.flush_rows()
        print('benchmark error: %s' % exc, file=sys.stderr, flush=True)
        rclpy.shutdown()
        return 1

    # Grace drain to catch in-flight messages.
    grace_end = time.monotonic() + args.grace_sec
    while time.monotonic() < grace_end:
        executor.spin_once(timeout_sec=0.05)
    executor.shutdown()
    # Single write of every buffered row, after the window closes.
    ctx.flush_rows()

    print('DONE sent=%d received=%d run_id=%s' % (
        sent, received, ctx.run_id), file=sys.stderr, flush=True)
    rclpy.shutdown()
    return 0


if __name__ == '__main__':
    sys.exit(main())
