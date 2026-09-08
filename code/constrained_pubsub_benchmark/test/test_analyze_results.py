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
    """Run ids split into the seven typed coordinates."""
    parsed = ar.parse_run_id(RUN_B)
    assert parsed == {
        'message': 'exp', 'config': 'constrained_pub_sub',
        'backend': 'xcdr', 'direction': 'cpp_to_cpp',
        'payload_bytes': 4000, 'target_frequency_hz': 10.0,
        'transport': 'shmem',
    }
    assert ar.parse_run_id(RUN_A)['target_frequency_hz'] == 10.0
    assert ar.parse_run_id(
        'std_copy_fastcdr__manual__40B__2p15443Hz__auto__pid123'
    )['target_frequency_hz'] == 2.15443


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


def test_sweep_and_summary_row_counts(tmp_path):
    """Sweep tables and the summary matrix aggregate as specified."""
    metrics, n_pub, n_recv = load_two_run_metrics(tmp_path)
    by_payload, by_frequency, by_direction = ar.aggregate_sweeps(metrics)
    assert len(by_payload) == 2 and len(by_frequency) == 2
    assert len(by_direction) == 2
    assert set(by_payload['n_runs']) == {1}
    matrix = ar.summary_matrix(metrics)
    assert len(matrix) == 2
    assert set(matrix['case']) == {
        'std_copy_fastcdr', 'exp_constrained_pub_sub_xcdr'}


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
    metrics, n_pub, n_recv = load_grid_metrics(tmp_path)
    out = os.path.join(str(tmp_path), 'figs')
    os.makedirs(out)
    matrix = ar.summary_matrix(metrics)
    heat = os.path.join(out, 'fig_summary_heatmap.png')
    ar.plot_summary_heatmap(matrix, heat)
    by_payload, by_frequency, _by_direction = ar.aggregate_sweeps(metrics)
    for case in ('std_copy_fastcdr', 'exp_constrained_pub_sub_xcdr'):
        payloads = sorted(
            by_payload.loc[by_payload['case'] == case, 'payload_bytes'])
        ar.plot_sweep(
            by_payload, case, 'payload_bytes', 'payload_bytes',
            payloads,
            os.path.join(out, 'fig_sweep_payload_%s.png' % case))
        freqs = sorted(
            by_frequency.loc[by_frequency['case'] == case,
                             'target_frequency_hz'])
        ar.plot_sweep(
            by_frequency, case, 'target_frequency_hz',
            'target_frequency_hz', freqs,
            os.path.join(out, 'fig_sweep_freq_%s.png' % case))
    ar.plot_time_series(
        metrics, 4000, 100.0, os.path.join(out, 'fig_timeseries.png'))
    for name in os.listdir(out):
        assert os.path.getsize(os.path.join(out, name)) > 0
    assert len(os.listdir(out)) == 6


def test_report_sections_and_links(tmp_path):
    """Report has all six sections with working figure references."""
    metrics, n_pub, n_recv = load_grid_metrics(tmp_path)
    out = os.path.join(str(tmp_path), 'analysis')
    os.makedirs(out)
    stats = ar.per_run_stats(metrics, n_pub, n_recv)
    matrix = ar.summary_matrix(metrics)
    drops = ar.drops_table(stats)
    figures = {
        'heatmap': 'fig_summary_heatmap.png',
        'payload_std_copy_fastcdr': 'fig_sweep_payload_std_copy_fastcdr.png',
        'payload_exp_constrained_pub_sub_xcdr':
            'fig_sweep_payload_exp_constrained_pub_sub_xcdr.png',
        'frequency_std_copy_fastcdr': 'fig_sweep_freq_std_copy_fastcdr.png',
        'frequency_exp_constrained_pub_sub_xcdr':
            'fig_sweep_freq_exp_constrained_pub_sub_xcdr.png',
        '400B@10Hz': 'fig_timeseries_400B_10Hz.png',
    }
    for name in figures.values():
        with open(os.path.join(out, name), 'w') as handle:
            handle.write('png')
    ar.write_report(
        out, matrix, stats, drops, ['400B@10Hz'], figures)
    report = open(os.path.join(out, 'report.md')).read()
    for section in ('## 1. Summary', '## 2. Latency by Payload Size',
                    '## 3. Latency by Publishing Frequency',
                    '## 4. Transport Comparison', '## 5. Time Series',
                    '## 6. Drop Analysis'):
        assert section in report
    assert 'NTP-synchronized' in report
    for name in figures.values():
        assert name in report


def test_end_to_end_cli(tmp_path):
    """Full CLI pipeline on the synthetic grid: six CSVs, figures, report."""
    import subprocess
    write_synthetic_grid(tmp_path)
    scripts = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'scripts')
    out = os.path.join(str(tmp_path), 'analysis')
    result = subprocess.run(
        [sys.executable,
         os.path.join(scripts, 'analyze_results.py'), str(tmp_path),
         '--output-dir', out],
        capture_output=True, text=True, check=False)
    assert result.returncode == 0, result.stderr
    import pandas as pd
    expected_rows = {
        'per_run_stats.csv': 32, 'summary_matrix.csv': 8,
        'sweep_by_payload.csv': 16, 'sweep_by_frequency.csv': 16,
        'sweep_by_direction.csv': 8, 'drops.csv': 0,
    }
    for name, count in expected_rows.items():
        path = os.path.join(out, name)
        assert os.path.isfile(path), name
        assert len(pd.read_csv(path)) == count, name
    for name in ('fig_summary_heatmap.png',
                 'fig_sweep_payload_std_copy_fastcdr.png',
                 'fig_sweep_payload_exp_constrained_pub_sub_xcdr.png',
                 'fig_sweep_freq_std_copy_fastcdr.png',
                 'fig_sweep_freq_exp_constrained_pub_sub_xcdr.png',
                 'fig_timeseries_400_100Hz.png',
                 'fig_timeseries_4K_100Hz.png',
                 'fig_timeseries_4K_10Hz.png',
                 'report.md'):
        path = os.path.join(out, name)
        assert os.path.isfile(path), name
        assert os.path.getsize(path) > 0, name
    report = open(os.path.join(out, 'report.md')).read()
    assert 'fig_summary_heatmap.png' in report


def test_loader_skips_outputs_and_strays(tmp_path):
    """Analysis outputs, stray CSVs, and empty files never pollute input."""
    make_two_run_matrix(tmp_path)
    with open(os.path.join(str(tmp_path), 'empty.csv'), 'w'):
        pass
    with open(os.path.join(str(tmp_path), 'notes.csv'), 'w') as handle:
        handle.write('a,b\n1,2\n')
    analysis = os.path.join(str(tmp_path), 'analysis')
    os.makedirs(analysis)
    with open(os.path.join(analysis, 'summary_matrix.csv'), 'w') as handle:
        handle.write('case,mean\nstd_copy_fastcdr,100.0\n')
    df = ar.load_raw_csvs(str(tmp_path), skip_dirs=(analysis,))
    assert set(df['run_id']) == {RUN_A, RUN_B}
    assert len(df) == 9
