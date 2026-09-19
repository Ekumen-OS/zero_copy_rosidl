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

"""Unit tests for scripts/analyze_results.py post-processing pipeline."""

import os
import sys

sys.path.insert(
    0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'scripts'))

import analyze_results as ar  # noqa: E402


COLUMNS = (
    'run_id,event,process,direction,config,message,backend,transport,'
    'payload_bytes,target_frequency_hz,step_index,phase,sequence,'
    'observed_utc,observed_mono_ns,send_mono_ns,receive_mono_ns,'
    'middleware_source_ns,middleware_received_ns,status,error'
)

RUN_A = 'std_copy_fastcdr__cpp_to_cpp__400B__10Hz__udp'
RUN_B = 'exp_constrained_pub_sub_xcdr__cpp_to_cpp__4000B__10Hz__shmem'


def pub_row(run_id, seq, send_ns, **overrides):
    """Build a publish row dict with sane defaults."""
    row = {
        'run_id': run_id, 'event': 'publish', 'process': 'pub',
        'direction': 'cpp_to_cpp', 'config': 'copy', 'message': 'std',
        'backend': 'fastcdr', 'transport': 'udp', 'payload_bytes': 400,
        'target_frequency_hz': 10.0, 'step_index': 0, 'phase': 'measured',
        'sequence': seq, 'observed_utc': '2026-09-07T00:00:00.000000Z',
        'observed_mono_ns': send_ns, 'send_mono_ns': send_ns,
        'receive_mono_ns': -1, 'middleware_source_ns': -1,
        'middleware_received_ns': -1, 'status': 'ok', 'error': '',
    }
    row.update(overrides)
    return row


def sub_row(run_id, seq, send_ns, recv_ns, **overrides):
    """Build a receive row dict with sane defaults."""
    row = dict(
        pub_row(run_id, seq, send_ns, **overrides), event='receive',
        process='sub', observed_mono_ns=recv_ns, receive_mono_ns=recv_ns,
        middleware_source_ns=send_ns + 10,
        middleware_received_ns=recv_ns - 10)
    return row


def write_csv(path, rows):
    """Write row dicts to path with the canonical header."""
    with open(path, 'w') as handle:
        handle.write(COLUMNS + '\n')
        for row in rows:
            handle.write(','.join(str(row[column]) for column in
                                  COLUMNS.split(',')) + '\n')


def make_two_run_matrix(tmp_path):
    """Write a synthetic 2-run matrix: run A perfect, run B one drop."""
    rows = []
    for seq, latency_us in enumerate((100.0, 200.0, 300.0)):
        send_ns = seq * 1000000
        rows.append(pub_row(RUN_A, seq, send_ns))
        rows.append(sub_row(RUN_A, seq, send_ns, send_ns + int(latency_us * 1000)))
    for seq, latency_us in enumerate((150.0, 250.0)):
        send_ns = seq * 1000000
        rows.append(pub_row(
            RUN_B, seq, send_ns, config='constrained_pub_sub',
            message='exp', backend='xcdr', transport='shmem',
            payload_bytes=4000))
        if seq == 0:
            rows.append(sub_row(
                RUN_B, seq, send_ns, send_ns + int(latency_us * 1000),
                config='constrained_pub_sub', message='exp', backend='xcdr',
                transport='shmem', payload_bytes=4000))
    path = os.path.join(str(tmp_path), 'matrix.csv')
    write_csv(path, rows)
    return path


def test_parse_run_id():
    """Run ids split into the eight typed coordinates."""
    parsed = ar.parse_run_id(RUN_B)
    assert parsed == {
        'message': 'exp', 'config': 'constrained_pub_sub',
        'backend': 'xcdr', 'direction': 'cpp_to_cpp',
        'payload_bytes': 4000, 'target_frequency_hz': 10.0,
        'transport': 'shmem', 'reliability': 'reliable',
    }
    assert ar.parse_run_id(RUN_A)['target_frequency_hz'] == 10.0
    assert ar.parse_run_id(
        'std_copy_fastcdr__manual__40B__2p15443Hz__auto__pid123'
    )['target_frequency_hz'] == 2.15443
    assert ar.parse_run_id(
        'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__best_effort'
    )['reliability'] == 'best_effort'
    assert ar.parse_run_id(
        'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__best_effort__pid7'
    )['reliability'] == 'best_effort'
    try:
        ar.parse_run_id(
            'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__sometimes')
    except ValueError:
        pass
    else:
        raise AssertionError('expected ValueError for bogus reliability')


def test_reliability_separates_runs(tmp_path):
    """Same coordinates differing only in reliability stay separate."""
    rows = []
    for seq in range(3):
        send_ns = seq * 100000000
        rows.append(pub_row(
            'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__reliable',
            seq, send_ns, config='copy', message='exp', backend='xcdr',
            transport='udp', payload_bytes=4000))
        rows.append(sub_row(
            'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__reliable',
            seq, send_ns, send_ns + 200000, config='copy', message='exp',
            backend='xcdr', transport='udp', payload_bytes=4000))
        rows.append(pub_row(
            'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__best_effort',
            seq, send_ns, config='copy', message='exp', backend='xcdr',
            transport='udp', payload_bytes=4000))
        rows.append(sub_row(
            'exp_copy_xcdr__cpp_to_cpp__4000B__10Hz__udp__best_effort',
            seq, send_ns, send_ns + 100000, config='copy', message='exp',
            backend='xcdr', transport='udp', payload_bytes=4000))
    path = os.path.join(str(tmp_path), 'rel.csv')
    write_csv(path, rows)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, n_pub, n_recv = ar.join_pub_sub(df)
    assert len(joined) == 6
    per_run = ar.per_run_stats(ar.derive_metrics(joined), n_pub, n_recv)
    assert len(per_run) == 2
    assert set(per_run['reliability']) == {'reliable', 'best_effort'}
    assert per_run.loc[
        per_run['reliability'] == 'reliable', 'mean'].iloc[0] == 200.0
    assert per_run.loc[
        per_run['reliability'] == 'best_effort', 'mean'].iloc[0] == 100.0
    pacing = ar.pacing_table(ar.derive_metrics(joined))
    assert set(pacing['reliability']) == {'reliable', 'best_effort'}


def test_load_filters_errors_and_warmup(tmp_path):
    """Warmup and error rows are dropped; legit seq-0 samples are kept."""
    rows = [
        pub_row(RUN_A, 0, 0),
        sub_row(RUN_A, 0, 0, 100000),
        dict(pub_row(RUN_A, 0, 0), phase='warmup'),
        dict(pub_row(RUN_A, 0, -1), event='error', status='error',
             error='boom'),
    ]
    path = os.path.join(str(tmp_path), 'mixed.csv')
    write_csv(path, rows)
    df = ar.load_raw_csvs(str(tmp_path))
    assert len(df) == 2
    assert set(df['event']) == {'publish', 'receive'}
    assert df['payload_bytes'].dtype.kind == 'i'


def test_join_counts_and_latencies(tmp_path):
    """Perfect run joins fully; the dropped sample is excluded."""
    make_two_run_matrix(tmp_path)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, n_pub, n_recv = ar.join_pub_sub(df)
    assert len(joined) == 4
    assert n_pub[RUN_A] == 3 and n_recv[RUN_A] == 3
    assert n_pub[RUN_B] == 2 and n_recv[RUN_B] == 1
    metrics = ar.derive_metrics(joined)
    lat_a = sorted(metrics.loc[
        metrics['run_id'] == RUN_A, 'latency_us'].tolist())
    assert lat_a == [100.0, 200.0, 300.0]
    lat_b = metrics.loc[metrics['run_id'] == RUN_B, 'latency_us'].tolist()
    assert lat_b == [150.0]
    mw_a = sorted(metrics.loc[
        metrics['run_id'] == RUN_A, 'middleware_latency_us'].tolist())
    assert mw_a == [99.98, 199.98, 299.98]
    elapsed = sorted(metrics.loc[
        metrics['run_id'] == RUN_A, 'elapsed_sec'].tolist())
    assert elapsed == [0.0, 0.001, 0.002]


def load_two_run_metrics(tmp_path):
    """Load the synthetic matrix through join and derive."""
    make_two_run_matrix(tmp_path)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, n_pub, n_recv = ar.join_pub_sub(df)
    return ar.derive_metrics(joined), n_pub, n_recv


def test_per_run_stats_known_values(tmp_path):
    """Hand-computed mean/p50/p99, achieved_hz, and drop rates."""
    metrics, n_pub, n_recv = load_two_run_metrics(tmp_path)
    stats = ar.per_run_stats(metrics, n_pub, n_recv)
    assert len(stats) == 2
    row_a = stats.loc[stats['run_id'] == RUN_A].iloc[0]
    assert row_a['n'] == 3
    assert row_a['mean'] == 200.0
    assert row_a['min'] == 100.0 and row_a['max'] == 300.0
    assert row_a['p50'] == 200.0 and row_a['p95'] == 290.0
    assert row_a['p99'] == 298.0
    # 3 samples over a 2 ms send window -> 1500 Hz.
    assert row_a['achieved_hz'] == 1500.0
    assert row_a['drop_rate'] == 0.0
    assert row_a['case'] == 'std_copy_fastcdr'
    row_b = stats.loc[stats['run_id'] == RUN_B].iloc[0]
    assert row_b['n'] == 1 and row_b['drop_rate'] == 0.5
    assert row_b['case'] == 'exp_constrained_pub_sub_xcdr'
    assert row_a['n_late'] == 0 and row_a['missed_frac'] == 0.0
    assert row_b['n_late'] == 0 and row_b['missed_frac'] == 0.5


def test_zero_variance_std_is_zero(tmp_path):
    """Identical latencies give std 0, not NaN."""
    rows = []
    for seq in range(3):
        rows.append(pub_row(RUN_A, seq, seq * 1000000))
        rows.append(sub_row(RUN_A, seq, seq * 1000000,
                            seq * 1000000 + 200000))
    path = os.path.join(str(tmp_path), 'flat.csv')
    write_csv(path, rows)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, n_pub, n_recv = ar.join_pub_sub(df)
    stats = ar.per_run_stats(ar.derive_metrics(joined), n_pub, n_recv)
    assert stats.iloc[0]['std'] == 0.0
    assert stats.iloc[0]['mean'] == 200.0


def test_streaming_collect_matches_direct(tmp_path):
    """Streaming per-file collection matches direct per-run stats."""
    metrics, n_pub, n_recv = load_two_run_metrics(tmp_path)
    direct = ar.per_run_stats(metrics, n_pub, n_recv)
    per_run, pacing, _pooled = ar.collect_per_run_tables(str(tmp_path))
    assert len(per_run) == len(direct) == 2
    assert set(per_run['case']) == {
        'std_copy_fastcdr', 'exp_constrained_pub_sub_xcdr'}
    assert set(per_run['run_id']) == set(direct['run_id'])
    assert len(pacing) == 2


def test_fixed_values_snap_to_figures(tmp_path):
    """Fixed values snap to figure-visible payloads, never excluded ones."""
    write_synthetic_grid(tmp_path)
    per_run, _pacing, _pooled = ar.collect_per_run_tables(str(tmp_path))
    assert len(per_run) == 32

    class Args:
        exclude_payloads = '4000'
        sweep_fixed_freq = None
        sweep_fixed_payload = None
        ts_case = None
        ts_direction = None
        figure_exclude_cases = None
        figure_exclude_directions = None
        line_exclude_cases = None
        throughput_xmin = None
    fixed = ar.resolve_fixed_values(per_run, Args())
    assert fixed['figure_exclude_payloads'] == [4000]
    # Medians resolve inside the visible set.
    assert fixed['sweep_fixed_payload'] == 400
    fig_only = ar.filter_figure_runs(
        per_run, exclude_payloads=fixed['figure_exclude_payloads'])
    assert set(fig_only['payload_bytes']) == {400}
    # Saturation scope keeps every payload.
    sat_only = ar.filter_figure_runs(per_run)
    assert set(sat_only['payload_bytes']) == {400, 4000}


def test_drops_table_only_dropped_runs(tmp_path):
    """Only runs with drops appear, with counts and rates."""
    metrics, n_pub, n_recv = load_two_run_metrics(tmp_path)
    stats = ar.per_run_stats(metrics, n_pub, n_recv)
    drops = ar.drops_table(stats)
    assert len(drops) == 1
    row = drops.iloc[0]
    assert row['run_id'] == RUN_B
    assert row['n_publish'] == 2 and row['n_receive'] == 1
    assert row['drops'] == 1 and row['drop_rate'] == 0.5


def test_pacing_table_known_values(tmp_path):
    """Inter-send stats and jitter ratio on hand-computed intervals."""
    rows = []
    for seq, send_ns in enumerate((0, 90000000, 210000000)):
        rows.append(pub_row(RUN_A, seq, send_ns))
        rows.append(sub_row(RUN_A, seq, send_ns, send_ns + 100000))
    path = os.path.join(str(tmp_path), 'paced.csv')
    write_csv(path, rows)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, _n_pub, _n_recv = ar.join_pub_sub(df)
    pacing = ar.pacing_table(ar.derive_metrics(joined))
    assert len(pacing) == 1
    row = pacing.iloc[0]
    assert row['n_intervals'] == 2
    assert row['nominal_period_ms'] == 100.0
    assert row['inter_send_mean_ms'] == 105.0
    assert round(row['inter_send_std_ms'], 3) == 21.213
    assert round(row['jitter_ratio'], 4) == 0.2020
    assert row['case'] == 'std_copy_fastcdr'
    summary = ar.pacing_summary_table(pacing)
    assert len(summary) == 1
    assert summary.iloc[0]['runs'] == 1
    assert round(summary.iloc[0]['mean'], 4) == 0.2020


def test_pacing_single_sample_is_nan(tmp_path):
    """One joined sample leaves intervals undefined, never crashes."""
    rows = [pub_row(RUN_A, 0, 0), sub_row(RUN_A, 0, 0, 100000)]
    path = os.path.join(str(tmp_path), 'single.csv')
    write_csv(path, rows)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, _n_pub, _n_recv = ar.join_pub_sub(df)
    pacing = ar.pacing_table(ar.derive_metrics(joined))
    row = pacing.iloc[0]
    assert row['n_intervals'] == 0
    assert row['inter_send_mean_ms'] != row['inter_send_mean_ms']  # NaN
    assert row['jitter_ratio'] != row['jitter_ratio']  # NaN


def test_pacing_figure_smoke(tmp_path):
    """Pacing figure renders with overlay, without it, and empty."""
    metrics, _n_pub, _n_recv = load_two_run_metrics(tmp_path)
    pacing = ar.pacing_table(metrics)
    out = os.path.join(str(tmp_path), 'pacing_figs')
    os.makedirs(out)
    ar.plot_pacing(
        pacing, 0.01, os.path.join(out, 'pacing_overlay.png'))
    ar.plot_pacing(pacing, 0.0, os.path.join(out, 'pacing_plain.png'))
    ar.plot_pacing(
        pacing.iloc[0:0], 0.01, os.path.join(out, 'pacing_empty.png'))
    for name in os.listdir(out):
        assert os.path.getsize(os.path.join(out, name)) > 0


def write_synthetic_grid(tmp_path, n_samples=12):
    """Write a 2x2x2x2x2 synthetic matrix for figure/report tests."""
    cases = [
        ('std', 'copy', 'fastcdr', 'udp'),
        ('std', 'copy', 'fastcdr', 'shmem'),
        ('exp', 'constrained_pub_sub', 'xcdr', 'udp'),
        ('exp', 'constrained_pub_sub', 'xcdr', 'shmem'),
    ]
    rows = []
    seq_global = 0
    for message, config, backend, transport in cases:
        for direction in ('cpp_to_cpp', 'intra_cpp'):
            for payload in (400, 4000):
                for freq in (10.0, 100.0):
                    run_id = '%s_%s_%s__%s__%dB__%s__%s' % (
                        message, config, backend, direction, payload,
                        ('%.6g' % freq).replace('.', 'p') + 'Hz',
                        transport)
                    base_latency = 100.0 + payload / 100.0
                    for seq in range(n_samples):
                        send_ns = seq * int(1e9 / freq)
                        recv_ns = send_ns + int(
                            (base_latency + seq) * 1000)
                        rows.append(pub_row(
                            run_id, seq, send_ns, process='pub',
                            direction=direction, config=config,
                            message=message, backend=backend,
                            transport=transport, payload_bytes=payload,
                            target_frequency_hz=freq))
                        rows.append(sub_row(
                            run_id, seq, send_ns, recv_ns,
                            process='sub', direction=direction,
                            config=config, message=message,
                            backend=backend, transport=transport,
                            payload_bytes=payload,
                            target_frequency_hz=freq))
                    seq_global += 1
    path = os.path.join(str(tmp_path), 'grid.csv')
    write_csv(path, rows)
    return path


def load_grid_metrics(tmp_path):
    """Load the synthetic grid through join and derive."""
    write_synthetic_grid(tmp_path)
    df = ar.load_raw_csvs(str(tmp_path))
    joined, n_pub, n_recv = ar.join_pub_sub(df)
    return ar.derive_metrics(joined), n_pub, n_recv


def test_formatters_round_trip():
    """Payload/frequency/step labels format and parse inversely."""
    assert ar.format_payload(40000) == '40K'
    assert ar.format_payload(4000000) == '4M'
    assert ar.format_payload(400) == '400'
    assert ar.parse_payload_label('40K') == 40000
    assert ar.step_label(40000, 100.0) == '40K@100Hz'
    assert ar.parse_step_label('40K@100Hz') == (40000, 100.0)
    assert ar.parse_step_label('4M@2p15443Hz') == (4000000, 2.15443)
    assert ar.format_us(284.0) == '284µs'
    assert ar.format_us(float('nan')) == 'n/a'


def test_figures_smoke(tmp_path):
    """All figure kinds render to non-empty files on synthetic data."""
    write_synthetic_grid(tmp_path)
    per_run, _pacing, _pooled = ar.collect_per_run_tables(str(tmp_path))
    per_run = ar.add_throughput_columns(per_run)
    out = os.path.join(str(tmp_path), 'figs')
    os.makedirs(out)
    transport, reliability = 'udp', 'reliable'
    ar.plot_sweep_payload(
        per_run, transport, reliability, 100.0,
        os.path.join(out, 'sweep_payload_lat.png'))
    ar.plot_sweep_frequency(
        per_run, transport, reliability, 4000,
        os.path.join(out, 'sweep_freq_lat.png'))
    # Synthetic offered loads top out at 400KB/s, below the default
    # 100MB/s zoom floor, so tests pin the floor at zero.
    ar.plot_throughput_vs_expected(
        per_run, transport, reliability,
        os.path.join(out, 'thr_vs_exp.png'), xmin_bps=0.0)
    ar.plot_latency_payload_by_transport(
        per_run, reliability,
        sorted(per_run['case'].unique())[0],
        sorted(per_run['direction'].unique())[0], 100.0,
        os.path.join(out, 'transport_lat.png'))
    ar.plot_throughput_frequency_by_transport(
        per_run, reliability,
        sorted(per_run['case'].unique())[0],
        sorted(per_run['direction'].unique())[0],
        os.path.join(out, 'transport_thr.png'), xmin_bps=0.0)
    ar.plot_corner_bars(
        _pooled, transport, reliability, 4000, 10.0,
        out_path=os.path.join(out, 'corner_bars.png'))
    ar.plot_ontime_vs_offered(
        per_run, transport, reliability,
        os.path.join(out, 'ontime.png'))
    max_frame = ar.throughput_max_frame(per_run)
    ar.plot_throughput_bars(
        max_frame, reliability, os.path.join(out, 'bars.png'))
    # Empty slices write nothing and report False instead of blank figures.
    assert ar.plot_sweep_payload(
        per_run, 'nonexistent', reliability, 100.0,
        os.path.join(out, 'empty.png')) is False
    assert not os.path.exists(os.path.join(out, 'empty.png'))
    for name in os.listdir(out):
        assert os.path.getsize(os.path.join(out, name)) > 0
    assert len(os.listdir(out)) == 8


def test_descriptive_throughput_labels():
    """Throughput legends read direction, backend, data path in full."""
    assert ar.descriptive_label(
        'exp_constrained_pub_only_xcdr',
        'cpp_to_py') == 'C++ to Python, XCDR, loan (write bounded)'
    assert ar.descriptive_label(
        'exp_constrained_pub_sub_xcdr',
        'py_to_py') == 'Python to Python, XCDR, loan (bounded)'
    assert ar.descriptive_label(
        'std_copy_fastcdr', 'cpp_to_cpp') == 'C++ to C++, FastCDR, copy'
    assert ar.descriptive_label(
        'std_copy_xcdr', 'cpp_to_cpp') == 'C++ to C++, XCDR, copy'


def test_case_short_labels():
    """Direction-cell legends carry backend + path only."""
    assert ar.case_short_label(
        'exp_constrained_pub_only_xcdr') == 'XCDR loan (write bounded)'
    assert ar.case_short_label(
        'exp_constrained_pub_sub_xcdr') == 'XCDR loan (bounded)'
    assert ar.case_short_label('std_copy_fastcdr') == 'FastCDR copy'
    assert ar.case_short_label('std_copy_xcdr') == 'XCDR copy'
    assert ar.case_short_label('exp_copy_xcdr') == 'XCDR copy'


def test_curves_coincide_removed():
    """Shared-handle merging is gone: every line keeps its own handle."""
    import numpy as np
    assert not hasattr(ar, 'merge_coincident_lines')
    assert not hasattr(ar, '_curves_coincide')
    assert not hasattr(ar, 'plot_pooled_boxes')
    assert not hasattr(ar, 'plot_missed_vs_offered')
    assert hasattr(ar, 'plot_corner_bars')
    assert hasattr(ar, 'plot_ontime_vs_offered')
    x, y = ar.max_same_x(
        np.array([10.0, 10.0, 20.0]), np.array([1.0, 3.0, 2.0]))
    assert list(y) == [3.0, 2.0]


def test_collapse_merge_and_xmin():
    """Shared-x points take the max; xmin/span helpers behave."""
    import numpy as np
    x, y = ar.max_same_x(
        np.array([10.0, 10.0, 20.0]), np.array([1.0, 3.0, 2.0]))
    assert list(x) == [10.0, 20.0]
    assert list(y) == [3.0, 2.0]
    assert ar.parse_throughput_label('100MB/s') == 100e6
    assert ar.parse_throughput_label('1GB/s') == 1e9
    assert ar.parse_throughput_label('40K') == 40000.0
    assert ar._span_subs([40.0], [4e10]) is None
    assert ar._span_subs([100.0], [200.0]) == (1.0, 2.0, 5.0)


def test_pooled_distributions_exact_on_synthetic(tmp_path):
    """Pooled histograms recover hand-computed quantiles."""
    write_synthetic_grid(tmp_path)
    per_run, _pacing, pooled = ar.collect_per_run_tables(str(tmp_path))
    # 2 transports x 1 reliability x 2 cases x 2 directions; payloads and
    # frequencies pool inside each group.
    assert len(pooled['groups']) == 8
    total = sum(e['n'] for e in pooled['groups'].values())
    assert total == 32 * 12
    md = ar.pooled_latency_table(pooled)
    assert 'p50' in md and 'Shared memory' in md
    key = ('udp', 'reliable', 'exp_constrained_pub_sub_xcdr', 'cpp_to_cpp')
    assert pooled['groups'][key]['n'] == 4 * 12
    assert len(pooled['groups'][key]['res']) > 0
    out = os.path.join(str(tmp_path), 'corner_bars.png')
    assert ar.plot_corner_bars(
        pooled, 'udp', 'reliable', 400, 10.0, out_path=out) is True
    assert os.path.getsize(out) > 0


def test_rendered_ticks_never_scientific(tmp_path):
    """Decade and subdivision ticks render SI on narrow and wide spans."""
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    cases = [
        (dict(x_kind='payload_bytes', y_kind='latency_us',
              x_ticks=[40, 40000000]),
         [40, 4e7], [70, 2e4]),
        (dict(x_kind='throughput_bps', y_kind='throughput_bps',
              subs=None),
         [40, 4e10], [40, 4e10]),
        (dict(x_kind='milliseconds', y_kind='milliseconds'),
         [1, 1000], [0.05, 500]),
        # Box-plot span: minors must stay silent, never math notation.
        (dict(x_kind='payload_bytes', y_kind='latency_us',
              x_ticks=[40, 4000]),
         [40, 4000], [15, 200]),
    ]
    for kwargs, xr, yr in cases:
        fig, ax = plt.subplots()
        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.plot(xr, yr)
        ar.apply_log_ticks(ax, **kwargs)
        fig.canvas.draw()
        labels = [t.get_text() for t in
                  list(ax.get_xticklabels()) + list(ax.get_yticklabels())]
        assert labels, kwargs
        for text in labels:
            assert 'e+' not in text and 'e-' not in text, (kwargs, text)
            assert '10^' not in text and '−' not in text, (kwargs, text)
        for axis in (ax.xaxis, ax.yaxis):
            for tick in axis.get_minor_ticks():
                assert tick.label1.get_text() == '', (
                    kwargs, tick.label1.get_text())
        plt.close(fig)


def test_throughput_includes_40M_without_averaging(tmp_path):
    """Throughput plots carry one point per run, 40M included."""
    write_synthetic_grid(tmp_path)
    per_run, _pacing, _pooled = ar.collect_per_run_tables(str(tmp_path))
    per_run = ar.add_throughput_columns(per_run)
    out = os.path.join(str(tmp_path), 'thr.png')
    assert ar.plot_throughput_vs_expected(
        per_run, 'udp', 'reliable', out, xmin_bps=0.0) is True
    assert os.path.getsize(out) > 0
    # 40M runs are kept in per-run tables and shown, never aggregated.
    assert 'achieved_throughput_Bps' in per_run.columns
    assert 'expected_throughput_Bps' in per_run.columns
    max_frame = ar.throughput_max_frame(per_run)
    thr_md = ar.throughput_max_table(max_frame)
    assert 'Max achieved' in thr_md
    assert 'At payload' in thr_md


def test_si_formatters_and_labels():
    """SI axis formatters and spelled-out labels behave."""
    assert ar.format_throughput_bps(40000) == '40KB/s'
    assert ar.format_throughput_bps(4000000) == '4MB/s'
    assert ar.format_freq_pretty(100.0) == '100 Hz'
    assert ar.humanize_case(
        'exp_constrained_pub_sub_xcdr') == 'Constrained pub/sub XCDR'
    assert ar.humanize_case('std_copy_fastcdr') == 'Copy FastCDR'
    assert ar.humanize_case('std_copy_xcdr') == 'Copy XCDR'
    assert ar.humanize_direction('cpp_to_py') == 'C++ to Python'
    assert ar.humanize_transport('shmem_ds') == \
        'Shared memory + data sharing'
    assert ar.axis_formatter('latency_us')(200.0, 0) == ar.format_us(200.0)
    assert ar.axis_formatter('payload_bytes')(40000.0, 0) == '40K'
    assert ar.axis_formatter('throughput_bps')(40000.0, 0) == '40KB/s'
    assert ar.axis_formatter('throughput_bps')(1000000000.0, 0) == '1GB/s'
    assert ar.axis_formatter('throughput_bps')(4e13, 0) == '40TB/s'
    assert ar.axis_formatter('latency_us')(0.5, 0) == '0.5µs'
    assert ar.axis_formatter('milliseconds')(10.0, 0) == '10ms'
    assert ar.axis_formatter('milliseconds')(0.5, 0) == '500µs'


def test_throughput_columns_and_exclusions(tmp_path):
    """Throughput math is exact; figure exclusions only affect figures."""
    write_synthetic_grid(tmp_path)
    per_run, _pacing, _pooled = ar.collect_per_run_tables(str(tmp_path))
    per_run = ar.add_throughput_columns(per_run)
    row = per_run.iloc[0]
    assert row['achieved_throughput_Bps'] == \
        row['achieved_hz'] * row['payload_bytes']
    assert row['expected_throughput_Bps'] == \
        row['target_frequency_hz'] * row['payload_bytes']
    fig_only = ar.filter_figure_runs(
        per_run, exclude_cases=('std_copy_fastcdr',),
        exclude_directions=('cpp_to_cpp',))
    assert len(fig_only) < len(per_run)
    assert 'std_copy_fastcdr' not in set(fig_only['case'])
    assert 'cpp_to_cpp' not in set(fig_only['direction'])
    assert len(per_run) == 32


def test_report_sections_and_links(tmp_path):
    """Restructured report has per-run sections with figure references."""
    metrics, n_pub, n_recv = load_grid_metrics(tmp_path)
    out = os.path.join(str(tmp_path), 'analysis')
    os.makedirs(out)
    stats = ar.per_run_stats(metrics, n_pub, n_recv)
    drops = ar.drops_table(stats)
    pacing_summary = ar.pacing_summary_table(ar.pacing_table(metrics))
    fixed = {
        'sweep_fixed_freq': 100.0, 'sweep_fixed_payload': 4000,
        'sweep_corner_payloads': [4000],
        'sweep_corner_freqs': [100.0],
        'ts_case': 'exp_constrained_pub_sub_xcdr',
        'ts_direction': 'cpp_to_cpp',
        'figure_exclude_cases': ['exp_copy_xcdr'],
        'figure_exclude_directions': ['intra_cpp', 'intra_py'],
        'figure_exclude_payloads': [],
        'line_exclude_cases': [],
        'throughput_xmin_bps': 100e6,
    }
    figures = {
        'sweep_payload_latency_100Hz_udp_reliable': 'sweep_lat.png',
        'sweep_freq_latency_4K_udp_reliable': 'sweep_freq.png',
        'throughput_vs_expected_udp_reliable': 'thr.png',
        'transport_latency_payload_reliable': 'cmp_lat.png',
        'corner_bars_4K_100Hz_udp_reliable': 'corner_bars.png',
        'ontime_vs_offered_udp_reliable': 'ontime.png',
        'throughput_bars_reliable': 'bars.png',
        'pacing': 'fig_pacing.png',
    }
    for name in figures.values():
        with open(os.path.join(out, name), 'w') as handle:
            handle.write('png')
    import numpy as np
    pooled = ar.new_pooled()
    ar.accumulate_pooled(pooled, metrics, np.random.default_rng(0))
    max_frame = ar.throughput_max_frame(
        ar.add_throughput_columns(stats))
    ar.write_report(
        out, stats, drops, pacing_summary, figures, fixed, pooled,
        max_frame)
    report = open(os.path.join(out, 'report.md')).read()
    for section in ('## 1. Dataset overview',
                    '## 2. Latency sweeps by payload and frequency',
                    '## 3. Throughput (achieved vs expected)',
                    '## 4. Transport comparisons',
                    '## 5. Pooled distributions and throughput ceiling',
                     '## 6. Regime bars: latency at grid corners',
                     '## 7. Drop Analysis', '## 8. Pacing'):
        assert section in report
    assert 'NTP-synchronized' in report
    assert 'Time series' not in report
    assert 'Heatmap' not in report
    assert 'Copy FastCDR' in report
    for name in figures.values():
        assert name in report


def test_end_to_end_cli(tmp_path):
    """Full streaming CLI pipeline on the synthetic grid."""
    import subprocess
    write_synthetic_grid(tmp_path)
    scripts = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'scripts')
    out = os.path.join(str(tmp_path), 'analysis')
    result = subprocess.run(
        [sys.executable,
         os.path.join(scripts, 'analyze_results.py'), str(tmp_path),
         '--output-dir', out, '--throughput-xmin', '0B/s',
         '--line-exclude-cases', ''],
        capture_output=True, text=True, check=False)
    assert result.returncode == 0, result.stderr
    import pandas as pd
    expected_rows = {
        'per_run_stats.csv': 32, 'drops.csv': 0, 'pacing.csv': 32,
        'pacing_summary.csv': 8,
    }
    for name, count in expected_rows.items():
        path = os.path.join(out, name)
        assert os.path.isfile(path), name
        assert len(pd.read_csv(path)) == count, name
    for name in ('fig_sweep_payload_latency_10Hz_udp_reliable.png',
                 'fig_sweep_payload_latency_100Hz_udp_reliable.png',
                 'fig_sweep_freq_latency_4K_udp_reliable.png',
                 'fig_throughput_vs_expected_udp_reliable.png',
                 'fig_transport_latency_payload_reliable.png',
                 'fig_transport_throughput_freq_reliable.png',
                 'fig_corner_bars_4K_10Hz_udp_reliable.png',
                 'fig_corner_bars_4K_10Hz_shmem_reliable.png',
                 'fig_corner_bars_4K_100Hz_udp_reliable.png',
                 'fig_corner_bars_4K_100Hz_shmem_reliable.png',
                 'fig_ontime_vs_offered_udp_reliable.png',
                 'fig_ontime_vs_offered_shmem_reliable.png',
                 'fig_throughput_bars_reliable.png',
                 'report.md'):
        path = os.path.join(out, name)
        assert os.path.isfile(path), name
        assert os.path.getsize(path) > 0, name
    report = open(os.path.join(out, 'report.md')).read()
    assert 'per_run_stats.csv' in report
    assert 'Time series' not in report
    assert 'Heatmap' not in report


def test_loader_skips_outputs_and_strays(tmp_path):
    """Analysis outputs, stray CSVs, and empty files never pollute input."""
    make_two_run_matrix(tmp_path)
    with open(os.path.join(str(tmp_path), 'empty.csv'), 'w'):
        pass
    with open(os.path.join(str(tmp_path), 'notes.csv'), 'w') as handle:
        handle.write('a,b\n1,2\n')
    analysis = os.path.join(str(tmp_path), 'analysis')
    os.makedirs(analysis)
    with open(os.path.join(analysis, 'per_run_stats.csv'), 'w') as handle:
        handle.write('case,mean\nstd_copy_fastcdr,100.0\n')
    df = ar.load_raw_csvs(str(tmp_path), skip_dirs=(analysis,))
    assert set(df['run_id']) == {RUN_A, RUN_B}
    assert len(df) == 9
    per_run, _pacing, _pooled = ar.collect_per_run_tables(
        str(tmp_path), skip_dirs=(analysis,))
    assert set(per_run['run_id']) == {RUN_A, RUN_B}
