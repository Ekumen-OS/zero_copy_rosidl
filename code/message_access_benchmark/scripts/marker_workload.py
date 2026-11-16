# Copyright 2026 Ekumen Inc.
# SPDX-License-Identifier: Apache-2.0

"""Public Python message APIs exercised by the Google Benchmark runner."""

import math

from visualization_msgs.msg import Marker

FRAME = 'benchmark_frame'
NAMESPACE = 'benchmark_cubes'


def message_class(experimental):
    """Resolve the requested generated MarkerArray class without a fallback."""
    if experimental:
        from visualization_msgs.msg.experimental import MarkerArray
    else:
        from visualization_msgs.msg import MarkerArray
    return MarkerArray


def prepare(msg, count, experimental):
    """Size the marker container using each representation's public API."""
    if experimental:
        msg.markers.resize(count)
    else:
        msg.markers = [Marker() for _ in range(count)]


def create(cls, count, experimental):
    """Create and size an owning message."""
    msg = cls()
    prepare(msg, count, experimental)
    return msg


def write(msg, seed):
    """Update scalar members, retaining nested handles within each marker."""
    for i, marker in enumerate(msg.markers):
        marker.id = i
        marker.type = 1
        marker.action = 0
        pose = marker.pose
        position = pose.position
        position.x = seed + i
        position.y = seed
        position.z = 0.0
        orientation = pose.orientation
        orientation.x = 0.0
        orientation.y = 0.0
        orientation.z = 0.0
        orientation.w = 1.0
        scale = marker.scale
        scale.x = 1.0
        scale.y = 1.0
        scale.z = 1.0
        color = marker.color
        color.r = 0.25
        color.g = 0.5
        color.b = 0.75
        color.a = 1.0


def strings(msg, seed):
    """Assign fixed-length strings; seed keeps the common callable signature."""
    for marker in msg.markers:
        marker.header.frame_id = FRAME
        marker.ns = NAMESPACE


def fill(msg, seed):
    """Fill an already-sized message."""
    write(msg, seed)
    strings(msg, seed)


def read(msg, seed):
    """Read the same scalar subset as C++ and return an observable checksum."""
    total = 0.0
    for marker in msg.markers:
        pose = marker.pose
        position = pose.position
        scale = marker.scale
        color = marker.color
        total += (
            int(marker.id)
            + float(position.x)
            + float(position.y)
            + float(position.z)
            + float(pose.orientation.w)
            + float(scale.x)
            + float(scale.y)
            + float(scale.z)
            + float(color.r)
            + float(color.g)
            + float(color.b)
            + float(color.a)
        )
    return total


def validate(msg, count, seed):
    """Fail explicitly if a wrapper mutates a copy rather than the message."""
    if len(msg.markers) != count:
        raise ValueError(f'expected {count} markers, got {len(msg.markers)}')
    expected = count * (count - 1 + 2 * seed + 6.5)
    if not math.isclose(read(msg, seed), expected, rel_tol=0.0, abs_tol=1e-6):
        raise ValueError('MarkerArray checksum mismatch')
    for marker in msg.markers:
        if str(marker.header.frame_id) != FRAME or str(marker.ns) != NAMESPACE:
            raise ValueError('MarkerArray string assignment did not persist')
