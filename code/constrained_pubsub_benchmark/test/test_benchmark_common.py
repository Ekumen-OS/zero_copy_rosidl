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

"""Unit tests for scripts/benchmark_common.py shared benchmark helpers."""

from contextlib import redirect_stdout
import io
import os
import sys
import xml.etree.ElementTree as ET

sys.path.insert(
    0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'scripts'))

import benchmark_common as bc  # noqa: E402
import transport_config as tc  # noqa: E402


PROFILE_NS = {'p': 'http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles'}


def check_raises(fn, *args):
    """Assert that calling fn with args raises ValueError."""
    try:
        fn(*args)
    except ValueError:
        return
    raise AssertionError('expected ValueError from %r' % (fn,))


def test_case_axes():
    """Message, config, and backend are orthogonal fixed vocabularies."""
    assert bc.MESSAGES == ('std', 'exp')
    assert bc.CONFIGS == ('copy', 'constrained_pub_only', 'constrained_pub_sub')
    assert bc.BACKENDS == ('auto', 'fastcdr', 'xcdr')


def test_resolve_backend():
    """
    Explicit backend wins, otherwise the default applies.

    FastCDR needs the copy config.
    """
    assert bc.resolve_backend('std', 'copy', 'auto') == 'fastcdr'
    assert bc.resolve_backend('std', 'copy', 'xcdr') == 'xcdr'
    assert bc.resolve_backend('exp', 'copy', 'auto') == 'xcdr'
    assert bc.resolve_backend('exp', 'constrained_pub_sub', 'auto') == 'xcdr'
    assert bc.resolve_backend('exp', 'copy', 'fastcdr') == 'fastcdr'
    check_raises(bc.resolve_backend, 'exp', 'constrained_pub_only', 'fastcdr')
    check_raises(bc.resolve_backend, 'exp', 'constrained_pub_sub', 'fastcdr')


def test_resolve_config():
    """Message, config, plus explicit-or-default backend resolve as a triple."""
    args = bc.build_parser().parse_args(
        ['--message', 'exp', '--config', 'constrained_pub_sub'])
    assert bc.resolve_config(args) == ('exp', 'constrained_pub_sub', 'xcdr')
    args = bc.build_parser().parse_args(
        ['--message', 'std', '--config', 'copy', '--backend', 'xcdr'])
    assert bc.resolve_config(args) == ('std', 'copy', 'xcdr')


def test_resolve_config_rejects_invalid_cases():
    """Constrained configs need --message exp; fastcdr needs copy."""
    args = bc.build_parser().parse_args(['--config', 'constrained_pub_only'])
    check_raises(bc.resolve_config, args)
    args = bc.build_parser().parse_args(
        ['--message', 'exp', '--config', 'constrained_pub_sub',
         '--backend', 'fastcdr'])
    check_raises(bc.resolve_config, args)


def test_parser_new_flag_defaults():
    """New shared flags default to benign values."""
    args = bc.build_parser().parse_args([])
    assert args.message == 'std'
    assert args.config == 'copy'
    assert args.backend == 'auto'
    assert args.transport == 'auto'
    assert args.run_id == ''
    assert args.direction is None
    assert args.step_index == -1
    assert args.sweep_payloads == bc.DEFAULT_PAYLOAD_GRID
    assert args.sweep_freqs is None
    assert args.sweep_shm == bc.DEFAULT_TRANSPORT_GRID
    assert args.duration_per_step == 10.0


def test_payload_grid():
    """Payload grids parse K/KB/M/MB suffixes; bad input raises."""
    assert bc.parse_payload_grid('4,40,400,4K,40K,400K,4M,40M') == [
        4, 40, 400, 4000, 40000, 400000, 4000000, 40000000]
    check_raises(bc.parse_payload_grid, '')
    check_raises(bc.parse_payload_grid, '4K,,40')


def test_frequency_grid_default():
    """Default grid is 10^(k/3) Hz for k = 0..9, strictly increasing."""
    grid = bc.default_frequency_grid()
    assert len(grid) == 10
    assert grid[0] == 1.0
    assert grid[3] == 10.0
    assert grid[6] == 100.0
    assert grid[9] == 1000.0
    assert all(b > a for a, b in zip(grid, grid[1:]))


def test_frequency_grid_parse():
    """Custom grids parse; empty or non-positive entries raise."""
    assert bc.parse_frequency_grid('1,10,100,1000') == [1.0, 10.0, 100.0, 1000.0]
    check_raises(bc.parse_frequency_grid, '')
    check_raises(bc.parse_frequency_grid, '10,0')
    check_raises(bc.parse_frequency_grid, '10,-5')


def test_run_id():
    """Run ids encode coordinates deterministically; manual fallback works."""
    assert bc.format_frequency_hz(10.0) == '10Hz'
    assert bc.format_frequency_hz(1000.0) == '1000Hz'
    assert bc.make_run_id(
        'exp', 'constrained_pub_sub', 'xcdr', 'cpp_to_cpp', 400000, 10.0,
        'shmem') == \
        'exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem'
    assert bc.make_run_id(
        'std', 'copy', 'fastcdr', None, 40, 1.0, 'auto') == \
        'std_copy_fastcdr__manual__40B__1Hz__auto'
    auto_id = bc.auto_run_id(
        'exp', 'constrained_pub_sub', 'xcdr', 'cpp_to_cpp', 400000, 10.0,
        'shmem')
    assert auto_id.startswith(
        'exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem__pid')
    assert auto_id.endswith(str(os.getpid()))


def test_transport_profile_filenames():
    """Each transport maps to its XML profile; auto maps to no override."""
    assert tc.transport_profile_filename('auto') is None
    assert tc.transport_profile_filename('udp') == 'transport_udp.xml'
    assert tc.transport_profile_filename('shmem') == 'transport_shmem.xml'
    assert tc.transport_profile_filename('shmem_ds') == 'transport_shmem_ds.xml'
    try:
        tc.transport_profile_filename('bogus')
    except KeyError:
        pass
    else:
        raise AssertionError('expected KeyError for bogus transport')


def test_transport_env():
    """Profiles file is set; XML QoS only for shmem_ds, cleared otherwise."""
    env = tc.transport_env('udp', profile_dir='/p')
    assert env == {
        tc.PROFILES_FILE_ENV_VAR: '/p/transport_udp.xml',
        tc.USE_QOS_FROM_XML_ENV_VAR: None,
    }
    env = tc.transport_env('shmem_ds', profile_dir='/p')
    assert env == {
        tc.PROFILES_FILE_ENV_VAR: '/p/transport_shmem_ds.xml',
        tc.USE_QOS_FROM_XML_ENV_VAR: '1',
    }
    env = tc.transport_env('auto')
    assert env == {tc.PROFILES_FILE_ENV_VAR: None}


def profiles_dir():
    """Return the source-tree profiles directory for structural tests."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(here, '..', 'profiles')


def parse_profile(filename):
    """Parse a transport profile XML file, returning its root element."""
    path = os.path.join(profiles_dir(), filename)
    assert os.path.isfile(path), 'missing profile: ' + path
    return ET.parse(path).getroot()


def check_profile(filename, transport_id, transport_type, writer_kind,
                  reader_kind=None):
    """Validate transport, participant, and endpoint QoS of one profile."""
    if reader_kind is None:
        reader_kind = writer_kind
    root = parse_profile(filename)
    assert root.tag.endswith('dds')

    descriptors = root.findall(
        './p:profiles/p:transport_descriptors/p:transport_descriptor', PROFILE_NS)
    assert [(d.find('p:transport_id', PROFILE_NS).text,
             d.find('p:type', PROFILE_NS).text) for d in descriptors] == [
                 (transport_id, transport_type)]

    participants = [
        p for p in root.findall('./p:profiles/p:participant', PROFILE_NS)
        if p.get('is_default_profile') == 'true']
    assert len(participants) == 1
    rtps = participants[0].find('p:rtps', PROFILE_NS)
    assert rtps.find('p:useBuiltinTransports', PROFILE_NS).text == 'false'
    assert rtps.find(
        'p:userTransports/p:transport_id', PROFILE_NS).text == transport_id

    for role, expected in (
            ('data_writer', writer_kind), ('data_reader', reader_kind)):
        endpoints = [
            e for e in root.findall('./p:profiles/p:' + role, PROFILE_NS)
            if e.get('is_default_profile') == 'true']
        assert len(endpoints) == 1
        kind = endpoints[0].find('p:qos/p:data_sharing/p:kind', PROFILE_NS)
        assert kind is not None and kind.text == expected


def test_transport_profiles_structure():
    """All three profiles select one transport and pin data-sharing QoS."""
    check_profile(
        'transport_udp.xml', 'benchmark_udp_transport', 'UDPv4', 'OFF')
    check_profile(
        'transport_shmem.xml', 'benchmark_shm_transport', 'SHM', 'OFF')
    # shmem_ds defaults: writer AUTOMATIC + DYNAMIC (internal writers
    # construct, gracefully skip data sharing), reader OFF + PREALLOCATED
    # (deterministic). AUTOMATIC is rendered per topic below.
    check_profile(
        'transport_shmem_ds.xml', 'benchmark_shm_ds_transport', 'SHM',
        'AUTOMATIC', 'OFF')
    # shmem_ds defaults pin resizable history (XML QoS would otherwise
    # fall back to non-resizable PREALLOCATED pools).
    root = parse_profile('transport_shmem_ds.xml')
    writer = root.find(
        './p:profiles/p:data_writer[@is_default_profile="true"]', PROFILE_NS)
    reader = root.find(
        './p:profiles/p:data_reader[@is_default_profile="true"]', PROFILE_NS)
    assert writer.find('p:historyMemoryPolicy', PROFILE_NS).text == \
        'DYNAMIC_REUSABLE'
    assert reader.find('p:historyMemoryPolicy', PROFILE_NS).text == \
        'PREALLOCATED_WITH_REALLOC'


def scoped_endpoint(root, role, topic):
    """Return the topic-scoped endpoint QoS (kind, history policy)."""
    matches = [
        e for e in root.findall('./p:profiles/p:' + role, PROFILE_NS)
        if e.get('profile_name') == topic]
    assert len(matches) == 1
    kind = matches[0].find('p:qos/p:data_sharing/p:kind', PROFILE_NS)
    policy = matches[0].find('p:historyMemoryPolicy', PROFILE_NS)
    assert kind is not None and policy is not None
    return kind.text, policy.text


def test_render_topic_profile():
    """Rendered profiles scope role-specific QoS to the exact topic."""
    base = os.path.join(profiles_dir(), 'transport_shmem_ds.xml')
    topic = '/benchmark_123_4'
    # Publishing role: data-sharing capable writer, inert reader.
    root = ET.fromstring(tc.render_topic_profile(base, topic, 'pub'))
    assert scoped_endpoint(root, 'data_writer', topic) == (
        'AUTOMATIC', 'PREALLOCATED_WITH_REALLOC')
    assert scoped_endpoint(root, 'data_reader', topic) == (
        'OFF', 'PREALLOCATED_WITH_REALLOC')
    # Receiving role: dynamic writer and reader, both AUTOMATIC.
    root = ET.fromstring(tc.render_topic_profile(base, topic, 'sub'))
    assert scoped_endpoint(root, 'data_writer', topic) == (
        'AUTOMATIC', 'DYNAMIC_REUSABLE')
    assert scoped_endpoint(root, 'data_reader', topic) == (
        'AUTOMATIC', 'DYNAMIC_REUSABLE')
    # Unknown roles are rejected.
    try:
        tc.render_topic_profile(base, topic, 'bogus')
    except ValueError:
        pass
    else:
        raise AssertionError('expected ValueError for bogus role')
    # Defaults are untouched.
    defaults = [
        e for e in root.findall('./p:profiles/p:data_writer', PROFILE_NS)
        if e.get('is_default_profile') == 'true']
    assert len(defaults) == 1
    kind = defaults[0].find('p:qos/p:data_sharing/p:kind', PROFILE_NS)
    assert kind is not None and kind.text == 'AUTOMATIC'


class FakeStamp:
    """Minimal header stamp with sec/nanosec fields."""

    def __init__(self):
        """Initialize zero stamp."""
        self.sec = 0
        self.nanosec = 0


class FakeHeader:
    """Minimal header with stamp and frame_id fields."""

    def __init__(self):
        """Initialize zero header."""
        self.stamp = FakeStamp()
        self.frame_id = ''


class FakeImage:
    """Minimal Image shape for metadata codec tests."""

    def __init__(self):
        """Initialize empty message."""
        self.header = FakeHeader()
        self.encoding = ''
        self.data = b''


def test_csv_header():
    """Header has the 21 canonical columns in stable order."""
    assert bc.csv_header() == ','.join(bc.COLUMNS)
    assert len(bc.COLUMNS) == 21


def test_escape_csv_field():
    """Commas, quotes, and newlines trigger quoting; plain passes through."""
    assert bc.escape_csv_field('plain') == 'plain'
    assert bc.escape_csv_field('') == ''
    assert bc.escape_csv_field('a,b') == '"a,b"'
    assert bc.escape_csv_field('a"b') == '"a""b"'
    assert bc.escape_csv_field('a\nb') == '"a\nb"'


def test_event_keys():
    """Event enum values translate to the CSV labels in order."""
    assert bc.EVENT_KEYS == ('publish', 'receive', 'error')
    assert bc.Event.PUBLISH == 0
    assert bc.Event.RECEIVE == 1
    assert bc.Event.ERROR == 2


def test_format_sample():
    """One fixed sample serializes to the exact expected row."""
    sample = bc.new_sample(
        run_id='exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem',
        event='receive', process='sub', direction='cpp_to_cpp',
        config='constrained_pub_sub', message='exp', backend='xcdr',
        transport='shmem', payload_bytes=400000,
        target_frequency_hz=10.0, step_index=3, sequence=42,
        observed_utc='2026-09-06T21:30:00.123456Z',
        observed_mono_ns=1000, send_mono_ns=900, receive_mono_ns=950)
    assert bc.format_sample(sample) == (
        'exp_constrained_pub_sub_xcdr__cpp_to_cpp__400000B__10Hz__shmem,'
        'receive,sub,cpp_to_cpp,constrained_pub_sub,exp,xcdr,shmem,400000,10,3,'
        'measured,42,2026-09-06T21:30:00.123456Z,1000,900,950,-1,-1,ok,')


def test_format_sample_escapes_error():
    """Error text with commas and quotes is quoted in the row."""
    sample = bc.new_sample(event='error', status='error',
                           error='failed: a, "b"')
    row = bc.format_sample(sample)
    assert row.endswith(',error,"failed: a, ""b"""')


def test_clocks():
    """Monotonic clock is non-decreasing; UTC has the expected shape."""
    assert bc.mono_now_ns() <= bc.mono_now_ns()
    utc = bc.utc_now_iso8601()
    assert len(utc) == 27 and utc[10] == 'T' and utc.endswith('Z')


def test_sample_metadata_codec():
    """Sequence and send time round-trip through header fields."""
    msg = FakeImage()
    bc.encode_sample_metadata(msg, 42, 1234567890123456789)
    assert len(msg.header.frame_id) == 16
    assert msg.header.frame_id == 's000000000000042'
    seq, send_ns = bc.decode_sample_metadata(msg)
    assert (seq, send_ns) == (42, 1234567890123456789)


def test_sample_metadata_rejects_foreign():
    """Non-benchmark frame ids decode to (None, None); bad seq raises."""
    msg = FakeImage()
    msg.header.frame_id = 'benchmark'
    assert bc.decode_sample_metadata(msg) == (None, None)
    msg.header.frame_id = 's12345'
    assert bc.decode_sample_metadata(msg) == (None, None)
    check_raises(bc.encode_sample_metadata, msg, 10 ** 15, 0)


def test_experimental_message_compat():
    """
    Fill/codec/constraints work on real messages (needs built ROS env).

    Guards the new-bindings shims: is_bigendian takes int (not bool),
    frame_id reads back as a String view, and stamp fields read back as
    fixed-width scalars. No-op when sensor_msgs is not importable.
    """
    try:
        from sensor_msgs.msg import Image
        from sensor_msgs.msg.experimental import Image as ExperimentalImage
    except ImportError:
        return
    for cls in (Image, ExperimentalImage):
        msg = cls()
        bc.fill_image(msg, 400, 7, 0, 0, 1.0)
        bc.encode_sample_metadata(msg, 7, 1234567890123456789)
        assert bc.decode_sample_metadata(msg) == (7, 1234567890123456789)
        assert len(msg.data) == 400
    constraints = bc.make_image_constraints(400, strict=True)
    assert constraints.strict is True
    assert constraints.max_string_length == 256
    assert constraints.type_specific.data.size == 400


def make_context(**overrides):
    """Build a RunContext with explicit fields and sane defaults."""
    fields = {
        'run_id': 'r', 'process': 'pub', 'direction': 'cpp_to_cpp',
        'config': 'copy', 'message': 'std', 'backend': 'fastcdr',
        'transport': 'shmem', 'payload_bytes': 400,
        'target_frequency_hz': 10.0, 'step_index': -1,
        'expected_samples': 2}
    fields.update(overrides)
    return bc.RunContext(**fields)


def test_run_context_round_trip():
    """Buffered rows flush to the same CSV as format_sample."""
    ctx = make_context()
    assert len(ctx) == 0
    ctx.emit(bc.Event.PUBLISH, 7, 900, -1, -1, -1)
    ctx.emit(bc.Event.ERROR, 0, -1, -1, -1, -1, error='boom')
    assert len(ctx) == 2
    out = io.StringIO()
    with redirect_stdout(out):
        ctx.flush_rows()
    assert len(ctx) == 0
    lines = out.getvalue().splitlines()
    assert len(lines) == 2
    # Publish row: observed derives from send; UTC captured at emit time.
    # Columns: ...,step_index,phase,sequence,observed_utc,observed_mono_ns,
    # send_mono_ns,...,status,error
    cols = lines[0].split(',')
    assert cols[1] == 'publish' and cols[12] == '7'
    assert cols[4] == 'copy' and cols[5] == 'std' and cols[6] == 'fastcdr'
    assert cols[14] == cols[15] == '900' and cols[19] == 'ok'
    assert cols[13].endswith('Z')
    assert lines[1].split(',')[1] == 'error'
    assert lines[1].endswith(',error,boom')


def test_run_context_from_args():
    """from_args resolves ids, direction fallback, and pre-sizes."""
    args = bc.build_parser().parse_args(
        ['--message', 'exp', '--config', 'constrained_pub_sub',
         '--direction', 'cpp_to_cpp', '--payload-bytes', '4K',
         '--publish-rate-hz', '10'])
    ctx = bc.RunContext.from_args(
        args, process='sub', direction_fallback='', backend='xcdr',
        expected_samples=10, payload_bytes=4000)
    assert ctx.run_id.startswith(
        'exp_constrained_pub_sub_xcdr__cpp_to_cpp__4000B__10Hz__auto__pid')
    assert ctx.run_id.endswith(str(os.getpid()))
    assert ctx.direction == 'cpp_to_cpp'
    assert ctx.message == 'exp' and ctx.config == 'constrained_pub_sub'
    assert ctx.payload_bytes == 4000
    # Explicit run id wins.
    args = bc.build_parser().parse_args(['--run-id', 'custom'])
    ctx = bc.RunContext.from_args(
        args, process='pub', direction_fallback='intra_py',
        backend='fastcdr', expected_samples=1, payload_bytes=40)
    assert ctx.run_id == 'custom'
    assert ctx.direction == 'intra_py'


def test_run_context_derives_receive_observed():
    """Receive rows derive observed_mono_ns from the receive timestamp."""
    ctx = make_context(expected_samples=1)
    ctx.emit(bc.Event.RECEIVE, 3, 900, 950, 1, 2)
    out = io.StringIO()
    with redirect_stdout(out):
        ctx.flush_rows()
    cols = out.getvalue().splitlines()[0].split(',')
    assert cols[14] == '950' and cols[15] == '900' and cols[16] == '950'


def test_run_context_grows_past_estimate():
    """Exceeding the estimate grows the buffer without losing rows."""
    ctx = make_context(expected_samples=0)  # capacity = 8, then grows
    for seq in range(20):
        ctx.emit(bc.Event.PUBLISH, seq, seq, seq, -1, -1)
    assert len(ctx) == 20


def test_shmem_profiles_fit_40mb():
    """SHM profiles size the segment above the largest payload step."""
    for filename in ('transport_shmem.xml', 'transport_shmem_ds.xml'):
        root = parse_profile(filename)
        descriptor = root.find(
            './p:profiles/p:transport_descriptors/p:transport_descriptor',
            PROFILE_NS)
        max_msg = int(descriptor.find('p:maxMessageSize', PROFILE_NS).text)
        segment = int(descriptor.find('p:segment_size', PROFILE_NS).text)
        assert max_msg >= 40000000
        assert segment >= max_msg
