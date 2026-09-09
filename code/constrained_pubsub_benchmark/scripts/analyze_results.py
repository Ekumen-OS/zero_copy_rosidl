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
Analyze raw CSV output from the constrained pub/sub benchmark matrix.

Reads per-sample publish/receive rows, joins them on (run_id, sequence),
derives latency metrics, and produces statistical tables (CSV), PNG
figures, and a Markdown report. See PLAN_postprocessing.md for the full
specification.
"""

import os
import sys

import numpy as np
import pandas as pd


#: Columns parsed out of run_id (plus the raw run_id itself).
RUN_ID_PARTS = (
    'message', 'config', 'backend', 'direction', 'payload_bytes',
    'target_frequency_hz', 'transport', 'reliability',
)

#: Case identity: the (message, config, backend) triple, e.g.
#: 'exp_constrained_pub_sub_xcdr'.
CASE_AXES = ('message', 'config', 'backend')


def parse_run_id(run_id):
    """
    Split a run id into its typed coordinates.

    Format: {message}_{config}_{backend}__{direction}__{payload}B__
    {freq}Hz__{transport}__{reliability}, with an optional __pidN suffix
    for manual runs. Historical ids without the reliability coordinate
    default to 'reliable'. Returns a dict with RUN_ID_PARTS keys; raises
    ValueError on malformed input.
    """
    parts = run_id.split('__')
    if len(parts) == 7 and parts[6].startswith('pid'):
        parts = parts[:6]
    elif len(parts) == 6 and parts[5].startswith('pid'):
        parts = parts[:5]
    if len(parts) == 5:
        parts = parts + ['reliable']
    if len(parts) != 6:
        raise ValueError('malformed run_id: %r' % (run_id,))
    case, direction, payload_s, freq_s, transport, reliability = parts
    if not payload_s.endswith('B') or not freq_s.endswith('Hz'):
        raise ValueError('malformed run_id: %r' % (run_id,))
    if reliability not in ('reliable', 'best_effort'):
        raise ValueError('malformed run_id reliability: %r' % (run_id,))
    case_fields = case.split('_')
    if len(case_fields) < 3:
        raise ValueError('malformed run_id case: %r' % (run_id,))
    message = case_fields[0]
    backend = case_fields[-1]
    config = '_'.join(case_fields[1:-1])
    return {
        'message': message,
        'config': config,
        'backend': backend,
        'direction': direction,
        'payload_bytes': int(payload_s[:-1]),
        'target_frequency_hz': float(freq_s[:-2].replace('p', '.')),
        'transport': transport,
        'reliability': reliability,
    }


#: Raw-run columns required for a file to count as benchmark input.
#: Analysis outputs (summary tables) and stray CSVs are skipped.
REQUIRED_COLUMNS = frozenset(
    ('run_id', 'event', 'sequence', 'phase'))


def load_raw_csvs(results_dir, skip_dirs=()):
    """
    Load every raw CSV under results_dir into one DataFrame.

    Finds *.csv recursively (minus skip_dirs, e.g. a nested analysis
    output), parses run_id into typed RUN_ID_PARTS columns, and drops
    warmup rows plus error rows (explicit failures and undecodable
    foreign traffic, which carry sequence 0). Legit sequence-0
    publish/receive samples are kept: the filter keys on the event
    column, not the sequence number. Files without the raw-run
    columns (analysis outputs, strays) and empty files are skipped
    with a stderr note; malformed lines are skipped by the parser.
    """
    skip_dirs = {os.path.realpath(d) for d in skip_dirs}
    frames = []
    for root, _dirs, files in os.walk(results_dir):
        if os.path.realpath(root) in skip_dirs:
            continue
        for name in sorted(files):
            if not name.endswith('.csv'):
                continue
            path = os.path.join(root, name)
            try:
                frame = pd.read_csv(path, on_bad_lines='skip')
            except pd.errors.EmptyDataError:
                sys.stderr.write('skipping empty file: %s\n' % path)
                continue
            if not REQUIRED_COLUMNS.issubset(frame.columns):
                sys.stderr.write('skipping non-run file: %s\n' % path)
                continue
            frames.append(frame)
    if not frames:
        raise ValueError('no CSV files under %r' % (results_dir,))
    df = pd.concat(frames, ignore_index=True)
    df = df.loc[~df['run_id'].astype(str).str.startswith('run_id')].copy()
    parsed = df['run_id'].astype(str).map(parse_run_id).apply(pd.Series)
    for column in RUN_ID_PARTS:
        df[column] = parsed[column]
    df['payload_bytes'] = df['payload_bytes'].astype(np.int64)
    df['sequence'] = df['sequence'].astype(np.int64)
    keep = (df['phase'] != 'warmup') & (df['event'] != 'error')
    return df.loc[keep].reset_index(drop=True)


def join_pub_sub(df):
    """
    Split publish/receive rows and inner-join on (run_id, sequence).

    Returns (joined, n_publish, n_receive): the joined frame with
    _pub/_sub suffixed columns, plus per-run publish and receive
    counts (Series indexed by run_id) for drop accounting. Unmatched
    publishes are drops (excluded from latency); unmatched receives
    are ignored.
    """
    pub = df.loc[df['event'] == 'publish'].copy()
    sub = df.loc[df['event'] == 'receive'].copy()
    n_publish = pub.groupby('run_id')['sequence'].count()
    n_receive = sub.groupby('run_id')['sequence'].count()
    joined = pd.merge(
        pub, sub, on=['run_id', 'sequence'], how='inner',
        suffixes=('_pub', '_sub'), validate='m:1')
    return joined.reset_index(drop=True), n_publish, n_receive


def derive_metrics(joined):
    """
    Add per-sample latency metrics to a joined frame.

    Adds latency_us (primary, end-to-end), middleware_latency_us
    (secondary, NaN when middleware stamps are unavailable), and
    elapsed_sec (per-run offset from the first send). Sweep
    coordinates are carried from the publish side.
    """
    df = joined.copy()
    df['latency_us'] = (
        df['receive_mono_ns_sub'] - df['send_mono_ns_pub']) / 1000.0
    mw_src = df['middleware_source_ns_sub'].to_numpy(dtype=np.float64)
    mw_recv = df['middleware_received_ns_sub'].to_numpy(dtype=np.float64)
    with np.errstate(invalid='ignore'):
        mw_lat = np.where(
            (mw_src >= 0) & (mw_recv >= 0), (mw_recv - mw_src) / 1000.0,
            np.nan)
    df['middleware_latency_us'] = mw_lat
    t0 = df.groupby('run_id')['send_mono_ns_pub'].transform('min')
    df['elapsed_sec'] = (
        df['send_mono_ns_pub'] - t0) / 1e9
    return df


#: Latency percentiles reported in every statistics table.
PERCENTILES = (50, 95, 99)


def per_run_stats(metrics, n_publish, n_receive):
    """
    Compute per-run statistics, one row per run_id.

    Reports sample count, mean, std, min, max, p50/p95/p99 of
    latency_us, achieved_hz (joined samples over the send window),
    and drop_rate (1 - n_receive / n_publish from pre-join counts).
    Sweep coordinates ride along from the first sample.
    """
    grouped = metrics.groupby('run_id', observed=True)
    stats = grouped['latency_us'].agg(
        n='count', mean='mean', std='std', min='min', max='max',
        p50=lambda s: s.quantile(0.50), p95=lambda s: s.quantile(0.95),
        p99=lambda s: s.quantile(0.99))
    stats = stats.reset_index()
    span_sec = (
        grouped['send_mono_ns_pub'].max() -
        grouped['send_mono_ns_pub'].min()).to_numpy(dtype=np.float64) / 1e9
    stats['achieved_hz'] = stats['n'].to_numpy(dtype=np.float64) / np.where(
        span_sec > 0, span_sec, np.nan)
    coords = grouped[[
        'message_pub', 'config_pub', 'backend_pub', 'direction_pub',
        'payload_bytes_pub', 'target_frequency_hz_pub', 'transport_pub',
        'reliability_pub', 'step_index_pub']].first().reset_index()
    coords = coords.rename(columns={
        'message_pub': 'message', 'config_pub': 'config',
        'backend_pub': 'backend', 'direction_pub': 'direction',
        'payload_bytes_pub': 'payload_bytes',
        'target_frequency_hz_pub': 'target_frequency_hz',
        'transport_pub': 'transport',
        'reliability_pub': 'reliability',
        'step_index_pub': 'step_index'})
    stats = pd.merge(stats, coords, on='run_id', how='left')
    counts = pd.DataFrame({
        'n_publish': n_publish, 'n_receive': n_receive}).reset_index()
    stats = pd.merge(stats, counts, on='run_id', how='left')
    stats['drop_rate'] = 1.0 - stats['n_receive'] / stats['n_publish']
    stats['case'] = (
        stats['message'] + '_' + stats['config'] + '_' + stats['backend'])
    return stats


def aggregate_sweeps(metrics):
    """
    Aggregate joined samples across the collapsed sweep axis.

    Returns (by_payload, by_frequency, by_direction): each row reports
    mean, std, p50, p95, p99 of latency_us plus the contributing run
    count. by_payload collapses frequency, by_frequency collapses
    payload, by_direction collapses both (direction stays explicit).
    """
    metrics = metrics.copy()
    metrics['case'] = (
        metrics['message_pub'] + '_' + metrics['config_pub'] + '_' +
        metrics['backend_pub'])

    def aggregate(frame, keys):
        grouped = frame.groupby(keys, observed=True)
        table = grouped['latency_us'].agg(
            mean='mean', std='std',
            p50=lambda s: s.quantile(0.50),
            p95=lambda s: s.quantile(0.95),
            p99=lambda s: s.quantile(0.99))
        table = table.reset_index()
        table['n_runs'] = grouped['run_id'].nunique().to_numpy()
        return table

    by_payload = aggregate(
        metrics, ['case', 'message_pub', 'config_pub', 'backend_pub',
                  'direction_pub', 'transport_pub', 'reliability_pub',
                  'payload_bytes_pub'])
    by_frequency = aggregate(
        metrics, ['case', 'message_pub', 'config_pub', 'backend_pub',
                  'direction_pub', 'transport_pub', 'reliability_pub',
                  'target_frequency_hz_pub'])
    by_direction = aggregate(
        metrics, ['case', 'message_pub', 'config_pub', 'backend_pub',
                  'direction_pub', 'transport_pub', 'reliability_pub'])
    rename = {
        'message_pub': 'message', 'config_pub': 'config',
        'backend_pub': 'backend', 'direction_pub': 'direction',
        'transport_pub': 'transport', 'reliability_pub': 'reliability',
        'payload_bytes_pub': 'payload_bytes',
        'target_frequency_hz_pub': 'target_frequency_hz'}
    return (by_payload.rename(columns=rename),
            by_frequency.rename(columns=rename),
            by_direction.rename(columns=rename))


def summary_matrix(metrics):
    """
    Aggregate to one row per (case, direction, transport) combo.

    Same statistics as the sweep tables; empty combos are absent
    (NaN handling happens at plot/report time, which skips them).
    """
    metrics = metrics.copy()
    metrics['case'] = (
        metrics['message_pub'] + '_' + metrics['config_pub'] + '_' +
        metrics['backend_pub'])
    grouped = metrics.groupby(
        ['case', 'message_pub', 'config_pub', 'backend_pub',
         'direction_pub', 'transport_pub', 'reliability_pub'],
        observed=True)
    table = grouped['latency_us'].agg(
        n='count', mean='mean', std='std', min='min', max='max',
        p50=lambda s: s.quantile(0.50),
        p95=lambda s: s.quantile(0.95),
        p99=lambda s: s.quantile(0.99)).reset_index()
    table['n_runs'] = grouped['run_id'].nunique().to_numpy()
    return table.rename(columns={
        'message_pub': 'message', 'config_pub': 'config',
        'backend_pub': 'backend', 'direction_pub': 'direction',
        'transport_pub': 'transport',
        'reliability_pub': 'reliability'})


def drops_table(per_run):
    """
    Keep only runs with drops, with counts and rates.

    Columns: run_id, case, direction, transport, payload_bytes,
    target_frequency_hz, n_publish, n_receive, drops, drop_rate.
    """
    drops = per_run.loc[per_run['n_receive'] < per_run['n_publish']].copy()
    drops['drops'] = drops['n_publish'] - drops['n_receive']
    return drops[[
        'run_id', 'case', 'direction', 'transport', 'reliability',
        'payload_bytes', 'target_frequency_hz', 'n_publish', 'n_receive',
        'drops', 'drop_rate']].reset_index(drop=True)


def pacing_table(metrics):
    """
    Compute per-run publish pacing statistics, one row per run_id.

    Reports the nominal period (from the target frequency), the observed
    inter-send interval mean/std/min/max in ms, and jitter_ratio
    (std/mean, dimensionless).  For uniform +/-j dither the ratio tends
    to j/sqrt(3) (~0.029 at the harness default 0.01); an exact metronome
    scores ~0.  Runs with fewer than two joined samples yield NaN.
    Sweep coordinates ride along from the first sample.
    """
    rows = []
    for run_id, group in metrics.groupby('run_id', observed=True):
        send_ms = np.sort(group['send_mono_ns_pub'].to_numpy(
            dtype=np.float64)) / 1e6
        dt = np.diff(send_ms)
        rows.append({
            'run_id': run_id,
            'n_intervals': len(dt),
            'inter_send_mean_ms': np.mean(dt) if len(dt) else np.nan,
            'inter_send_std_ms':
                np.std(dt, ddof=1) if len(dt) > 1 else np.nan,
            'inter_send_min_ms': np.min(dt) if len(dt) else np.nan,
            'inter_send_max_ms': np.max(dt) if len(dt) else np.nan,
        })
    pacing = pd.DataFrame(rows)
    pacing['jitter_ratio'] = (
        pacing['inter_send_std_ms'] / pacing['inter_send_mean_ms'])
    coords = metrics.groupby('run_id', observed=True)[[
        'message_pub', 'config_pub', 'backend_pub', 'direction_pub',
        'transport_pub', 'reliability_pub', 'payload_bytes_pub',
        'target_frequency_hz_pub']].first().reset_index()
    coords = coords.rename(columns={
        'message_pub': 'message', 'config_pub': 'config',
        'backend_pub': 'backend', 'direction_pub': 'direction',
        'transport_pub': 'transport',
        'reliability_pub': 'reliability',
        'payload_bytes_pub': 'payload_bytes',
        'target_frequency_hz_pub': 'target_frequency_hz'})
    pacing = pd.merge(pacing, coords, on='run_id', how='left')
    pacing['nominal_period_ms'] = 1000.0 / pacing['target_frequency_hz']
    pacing['case'] = (
        pacing['message'] + '_' + pacing['config'] + '_' +
        pacing['backend'])
    return pacing[[
        'run_id', 'case', 'direction', 'transport', 'reliability',
        'payload_bytes', 'target_frequency_hz', 'nominal_period_ms',
        'n_intervals', 'inter_send_mean_ms', 'inter_send_std_ms',
        'inter_send_min_ms', 'inter_send_max_ms', 'jitter_ratio']]


def pacing_summary_table(pacing):
    """
    Aggregate pacing jitter per (case, direction, transport, reliability).

    Reports run count plus mean/worst jitter_ratio; compact enough to
    inline in the report.  Compare against the configured dither
    fraction (uniform +/-j tends to j/sqrt(3)).
    """
    return pacing.groupby(
        ['case', 'direction', 'transport', 'reliability'],
        observed=True)['jitter_ratio'].agg(
            runs='count', mean='mean',
            worst='max').reset_index()


#: Transport colors, used identically in every figure.
TRANSPORT_COLORS = {
    'udp': '#1f77b4',
    'shmem': '#ff7f0e',
    'shmem_ds': '#2ca02c',
}

#: Canonical direction order for subplot grids.
DIRECTIONS = (
    'cpp_to_cpp', 'cpp_to_py', 'py_to_cpp',
    'py_to_py', 'intra_cpp', 'intra_py',
)


def ensure_backend():
    """Select the headless matplotlib backend (safe to call repeatedly)."""
    import matplotlib
    matplotlib.use('Agg')


def format_us(value):
    """Format a microsecond latency as a linear human string."""
    if not np.isfinite(value):
        return 'n/a'
    if value < 1000.0:
        return '%gµs' % value
    if value < 1000000.0:
        return '%gms' % (value / 1000.0)
    return '%gs' % (value / 1000000.0)


def format_payload(payload_bytes):
    """Format a payload size with K/M suffixes (40000 -> '40K')."""
    payload_bytes = int(payload_bytes)
    if payload_bytes >= 1000000 and payload_bytes % 1000000 == 0:
        return '%gM' % (payload_bytes / 1000000)
    if payload_bytes >= 1000 and payload_bytes % 1000 == 0:
        return '%gK' % (payload_bytes / 1000)
    return str(payload_bytes)


def parse_payload_label(label):
    """Inverse of format_payload ('40K' -> 40000)."""
    label = label.strip()
    if label.endswith('M'):
        return int(float(label[:-1]) * 1000000)
    if label.endswith('K'):
        return int(float(label[:-1]) * 1000)
    return int(label)


def format_freq_hz(hz):
    """Format a frequency like the benchmark run ids ('10Hz', '2p15443Hz')."""
    return ('%.6g' % hz).replace('.', 'p') + 'Hz'


def parse_freq_label(label):
    """Inverse of format_freq_hz ('2p15443Hz' -> 2.15443)."""
    label = label.strip()
    if label.endswith('Hz'):
        label = label[:-2]
    return float(label.replace('p', '.'))


def step_label(payload_bytes, frequency_hz):
    """Build a time-series step label ('40K@100Hz')."""
    return '%s@%s' % (format_payload(payload_bytes),
                      format_freq_hz(frequency_hz))


def parse_step_label(label):
    """Split a step label into (payload_bytes, frequency_hz)."""
    payload_s, _, freq_s = label.partition('@')
    return parse_payload_label(payload_s), parse_freq_label(freq_s)


def default_time_series_steps(payloads, freqs, exclude_payloads=()):
    """
    Build the default time-series step set.

    One step per payload (minus exclusions) at the reference frequency
    (100 Hz when gridded, else the median), plus one step per frequency
    at the reference payload (40K when gridded, else the median).
    """
    payloads = sorted(set(payloads) - set(exclude_payloads))
    freqs = sorted(set(freqs))
    ref_freq = min(freqs, key=lambda f: abs(f - 100.0))
    ref_payload = min(payloads, key=lambda p: abs(p - 40000))
    steps = [step_label(p, ref_freq) for p in payloads]
    steps += [step_label(ref_payload, f) for f in freqs]
    return steps


def plot_summary_heatmap(summary, out_path, dpi=150):
    """
    Write the per-transport mean-latency heatmap.

    One panel per transport present in the data; rows are cases,
    columns are directions. Log-normalized viridis cells annotated
    with linear values; missing combos stay blank.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm
    transports = sorted(summary['transport'].unique())
    cases = sorted(summary['case'].unique())
    reliabilities = sorted(summary['reliability'].unique())
    rows = [(case, reliability)
            for case in cases for reliability in reliabilities]
    directions = [d for d in DIRECTIONS if d in set(summary['direction'])]
    width = max(6.0, 3.0 * len(directions))
    height = max(8.0, 2.0 + 1.2 * len(rows))
    fig, axes = plt.subplots(
        1, len(transports), figsize=(width * len(transports) / 2 + 6,
                                     height),
        squeeze=False)
    for ax, transport in zip(axes[0], transports):
        panel = summary.loc[summary['transport'] == transport]
        grid = panel.pivot_table(
            values='mean', index=['case', 'reliability'],
            columns='direction')
        grid = grid.reindex(
            index=pd.MultiIndex.from_tuples(
                rows, names=['case', 'reliability']),
            columns=directions)
        values = grid.to_numpy(dtype=np.float64)
        positive = values[np.isfinite(values) & (values > 0)]
        norm = LogNorm(vmin=positive.min(), vmax=positive.max()) \
            if positive.size else None
        mesh = ax.imshow(values, aspect='auto', cmap='viridis', norm=norm)
        ax.set_xticks(range(len(directions)), directions, rotation=45,
                      ha='right')
        ax.set_yticks(range(len(rows)),
                      ['%s %s' % row for row in rows])
        ax.set_title(transport)
        for (row, col), value in np.ndenumerate(values):
            if np.isfinite(value):
                ax.text(col, row, format_us(value), ha='center',
                        va='center', fontsize=8, color='white')
        fig.colorbar(mesh, ax=ax, label='mean latency_us (log)')
    fig.suptitle('Mean end-to-end latency by case, direction, transport')
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)


def plot_sweep(table, case, x_column, x_label, x_ticks, out_path, dpi=150):
    """
    Write one 2x3 per-direction sweep figure for a case.

    One line per transport (mean across the collapsed axis) with a
    ±1 stddev envelope; both axes log-scaled. Directions absent from
    the data leave blank subplots.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    subset = table.loc[table['case'] == case]
    directions = [d for d in DIRECTIONS if d in set(subset['direction'])]
    fig, axes = plt.subplots(2, 3, figsize=(18, 12), squeeze=False)
    for ax, direction in zip(axes.flat, directions):
        frame = subset.loc[subset['direction'] == direction].sort_values(
            x_column)
        for transport, color in TRANSPORT_COLORS.items():
            for reliability, linestyle in (('reliable', '-'),
                                           ('best_effort', '--')):
                line = frame.loc[
                    (frame['transport'] == transport) &
                    (frame['reliability'] == reliability)].sort_values(
                        x_column)
                if line.empty:
                    continue
                x = line[x_column].to_numpy(dtype=np.float64)
                mean = line['mean'].to_numpy(dtype=np.float64)
                std = line['std'].to_numpy(dtype=np.float64)
                ax.plot(x, mean, label='%s %s' % (transport, reliability),
                        color=color, linestyle=linestyle, linewidth=2)
                ax.fill_between(x, mean - std, mean + std, color=color,
                                alpha=0.3)
        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.set_xlabel(x_label)
        ax.set_ylabel('latency_us (log)')
        ax.set_title(direction)
        ax.set_xticks(x_ticks)
        ax.get_xaxis().set_major_formatter(plt.ScalarFormatter())
        ax.grid(True, which='both', alpha=0.3)
    legend_ax = axes[0][2] if len(directions) > 2 else axes.flat[
        len(directions) - 1]
    handles, _labels = legend_ax.get_legend_handles_labels()
    if handles:
        legend_ax.legend(loc='upper right')
    for ax in axes.flat[len(directions):]:
        ax.set_visible(False)
    fig.suptitle('%s: latency by %s' % (case, x_label))
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)


def plot_time_series(metrics, payload_bytes, frequency_hz, out_path,
                     dpi=150):
    """
    Write one per-direction time-series figure for a payload@freq step.

    Each direction subplot shows one line per transport (per-sample
    latency over elapsed seconds) with a p1/p99 outer envelope and a
    ±1 stddev inner envelope from rolling windows.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    subset = metrics.loc[
        (metrics['payload_bytes_pub'] == payload_bytes) &
        (np.isclose(metrics['target_frequency_hz_pub'], frequency_hz))]
    directions = [d for d in DIRECTIONS if d in set(subset['direction_pub'])]
    fig, axes = plt.subplots(2, 3, figsize=(18, 10), squeeze=False)
    for ax, direction in zip(axes.flat, directions):
        frame = subset.loc[subset['direction_pub'] == direction]
        for transport, color in TRANSPORT_COLORS.items():
            for reliability, linestyle in (('reliable', '-'),
                                           ('best_effort', '--')):
                line = frame.loc[
                    (frame['transport_pub'] == transport) &
                    (frame['reliability_pub'] == reliability)].sort_values(
                        'elapsed_sec')
                if line.empty:
                    continue
                x = line['elapsed_sec'].to_numpy(dtype=np.float64)
                y = line['latency_us'].to_numpy(dtype=np.float64)
                window = max(1, len(y) // 100)
                series = pd.Series(y)
                roll = series.rolling(window, min_periods=1)
                p1 = roll.quantile(0.01).to_numpy()
                p99 = roll.quantile(0.99).to_numpy()
                mean = roll.mean().to_numpy()
                std = roll.std(ddof=0).fillna(0.0).to_numpy()
                ax.plot(x, y, color=color, linestyle=linestyle, alpha=0.4,
                        linewidth=0.8)
                ax.fill_between(x, p1, p99, color=color, alpha=0.05)
                ax.fill_between(mean - std, mean + std, color=color,
                                alpha=0.2)
                ax.plot([], [], label='%s %s' % (transport, reliability),
                        color=color, linestyle=linestyle, linewidth=2)
        ax.set_xlabel('elapsed_sec')
        ax.set_ylabel('latency_us')
        ax.set_title(direction)
        handles, _labels = ax.get_legend_handles_labels()
        if handles:
            ax.legend(loc='upper right')
        ax.grid(True, alpha=0.3)
    for ax in axes.flat[len(directions):]:
        ax.set_visible(False)
    fig.suptitle('Latency over time: %s' %
                 step_label(payload_bytes, frequency_hz))
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)


def plot_pacing(pacing, expected_jitter, out_path, dpi=150):
    """
    Write the publish-pacing scatter.

    Observed inter-send std vs nominal period, one marker per run,
    colored by transport. Overlays the uniform-dither reference (std =
    period * jitter / sqrt(3)) when expected_jitter > 0, so a
    misconfigured or missing dither shows as a systematic departure. An
    exact metronome sits orders of magnitude below the line; NaN rows
    are skipped.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(10, 6))
    ok = pacing.dropna(
        subset=['nominal_period_ms', 'inter_send_std_ms'])
    reliabilities = sorted(ok['reliability'].unique())
    markers = ('o', '^', 's', 'D')
    for transport, color in TRANSPORT_COLORS.items():
        for pos, reliability in enumerate(reliabilities):
            pts = ok.loc[(ok['transport'] == transport) &
                         (ok['reliability'] == reliability)]
            if pts.empty:
                continue
            ax.scatter(pts['nominal_period_ms'], pts['inter_send_std_ms'],
                       label='%s %s' % (transport, reliability),
                       color=color, marker=markers[pos % len(markers)],
                       alpha=0.6)
    if expected_jitter > 0 and not ok.empty:
        lo = ok['nominal_period_ms'].min()
        hi = ok['nominal_period_ms'].max()
        span = np.logspace(np.log10(lo), np.log10(max(hi, lo * 1.01)))
        ax.plot(span, span * expected_jitter / np.sqrt(3), 'k--',
                label='uniform +/-%.2g' % expected_jitter)
    elif ok.empty:
        ax.text(0.5, 0.5, 'no pacing data', ha='center', va='center',
                transform=ax.transAxes)
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('nominal period_ms (log)')
    ax.set_ylabel('inter-send std_ms (log)')
    handles, _labels = ax.get_legend_handles_labels()
    if handles:
        ax.legend(loc='upper left')
    ax.grid(True, which='both', alpha=0.3)
    fig.suptitle('Publish pacing jitter: observed vs configured')
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)


def markdown_table(frame):
    """
    Render a DataFrame as a GitHub-flavored markdown table.

    Avoids the tabulate dependency; NaN renders as 'n/a'.
    """
    columns = list(frame.columns)
    rows = [[('n/a' if (v != v) else str(v)) for v in frame[column]]
            for column in columns]
    lines = ['| ' + ' | '.join(columns) + ' |',
             '| ' + ' | '.join(['---'] * len(columns)) + ' |']
    for i in range(len(frame)):
        lines.append('| ' + ' | '.join(row[i] for row in rows) + ' |')
    return '\n'.join(lines)


def transport_deltas(summary):
    """
    Compute shmem_ds-vs-shmem and shmem-vs-udp deltas per (case, direction).

    Returns a frame with mean and p95 differences in latency_us; pairs
    missing either transport are dropped.
    """
    pivot_mean = summary.pivot_table(
        values='mean', index=['case', 'direction', 'reliability'],
        columns='transport')
    pivot_p95 = summary.pivot_table(
        values='p95', index=['case', 'direction', 'reliability'],
        columns='transport')
    deltas = pd.DataFrame(index=pivot_mean.index)
    if 'shmem_ds' in pivot_mean and 'shmem' in pivot_mean:
        deltas['ds_minus_shmem_mean'] = (
            pivot_mean['shmem_ds'] - pivot_mean['shmem'])
        deltas['ds_minus_shmem_p95'] = (
            pivot_p95['shmem_ds'] - pivot_p95['shmem'])
    if 'shmem' in pivot_mean and 'udp' in pivot_mean:
        deltas['shmem_minus_udp_mean'] = (
            pivot_mean['shmem'] - pivot_mean['udp'])
        deltas['shmem_minus_udp_p95'] = (
            pivot_p95['shmem'] - pivot_p95['udp'])
    return deltas.dropna(how='all').reset_index()


def reliability_deltas(summary):
    """
    Compute reliable-vs-best_effort deltas per (case, direction, transport).

    Returns a frame with mean and p95 differences in latency_us;
    positive favors best_effort (reliable minus best_effort).  Combos
    missing either level are dropped.
    """
    pivot_mean = summary.pivot_table(
        values='mean', index=['case', 'direction', 'transport'],
        columns='reliability')
    pivot_p95 = summary.pivot_table(
        values='p95', index=['case', 'direction', 'transport'],
        columns='reliability')
    deltas = pd.DataFrame(index=pivot_mean.index)
    if 'reliable' in pivot_mean and 'best_effort' in pivot_mean:
        deltas['rel_minus_be_mean'] = (
            pivot_mean['reliable'] - pivot_mean['best_effort'])
        deltas['rel_minus_be_p95'] = (
            pivot_p95['reliable'] - pivot_p95['best_effort'])
    return deltas.dropna(how='all').reset_index()


def write_report(output_dir, summary, per_run, drops, steps, figures,
                 pacing_summary, image_format='png'):
    """
    Write report.md with the seven plan sections.

    Embeds relative figure paths, inlines the summary matrix as a
    markdown table, and states the clock-sync assumption. figures maps
    logical names to file basenames present in output_dir.
    """
    lines = []
    lines.append('# Benchmark Analysis Report')
    lines.append('')
    lines.append('## 1. Summary')
    lines.append('')
    lines.append('Mean end-to-end latency (µs) per (case, direction, '
                 'transport):')
    lines.append('')
    show = summary[[
        'case', 'direction', 'transport', 'reliability', 'n_runs', 'mean',
        'p50', 'p95', 'p99']].copy()
    for column in ('mean', 'p50', 'p95', 'p99'):
        show[column] = show[column].map(format_us)
    lines.append(markdown_table(show))
    lines.append('')
    lines.append('> Key findings: compare transports within each '
                 '(case, direction) row; see §4 for deltas. '
                 'Cross-process end-to-end latency assumes '
                 'NTP-synchronized host clocks.')
    lines.append('')
    lines.append('## 2. Latency by Payload Size')
    lines.append('')
    for case in sorted(summary['case'].unique()):
        lines.append('### %s' % case)
        lines.append('')
        lines.append('![payload sweep](%s)' %
                     figures['payload_%s' % case])
        lines.append('')
        for direction in sorted(
                summary.loc[summary['case'] == case, 'direction'].unique()):
            lines.append('#### %s' % direction)
            lines.append('')
            table = summary.loc[(summary['case'] == case) &
                                (summary['direction'] == direction),
                                ['transport', 'reliability', 'n_runs',
                                 'mean', 'p50', 'p95']]
            lines.append(markdown_table(table))
            lines.append('')
    lines.append('## 3. Latency by Publishing Frequency')
    lines.append('')
    for case in sorted(summary['case'].unique()):
        lines.append('### %s' % case)
        lines.append('')
        lines.append('![frequency sweep](%s)' %
                     figures['frequency_%s' % case])
        lines.append('')
    lines.append('## 4. Transport Comparison')
    lines.append('')
    lines.append('![summary heatmap](%s)' % figures['heatmap'])
    lines.append('')
    deltas = transport_deltas(summary)
    if len(deltas):
        lines.append('Mean/p95 deltas (µs; negative favors the first '
                     'transport):')
        lines.append('')
        lines.append(markdown_table(deltas))
        lines.append('')
    else:
        lines.append('Not enough overlapping transports for deltas.')
        lines.append('')
    rel_deltas = reliability_deltas(summary)
    if len(rel_deltas):
        lines.append('Reliable-vs-best_effort deltas (µs; positive favors '
                     'best_effort):')
        lines.append('')
        lines.append(markdown_table(rel_deltas))
        lines.append('')
    else:
        lines.append('Single reliability level: no reliability deltas.')
        lines.append('')
    lines.append('## 5. Time Series')
    lines.append('')
    for label in steps:
        lines.append('### %s' % label)
        lines.append('')
        lines.append('![time series %s](%s)' % (label, figures[label]))
        lines.append('')
    lines.append('## 6. Drop Analysis')
    lines.append('')
    if len(drops):
        lines.append('Runs with drops > 0:')
        lines.append('')
        lines.append(markdown_table(drops))
        lines.append('')
        by_transport = drops.groupby(
            ['transport', 'reliability', 'case'],
            observed=True)['drop_rate'].agg(
                runs='count', mean='mean', worst='max').reset_index()
        lines.append('Drop-rate summary per (transport, reliability, case):')
        lines.append('')
        lines.append(markdown_table(by_transport))
        lines.append('')
    else:
        lines.append('No drops recorded.')
        lines.append('')
    lines.append('## 7. Pacing and Publish Jitter')
    lines.append('')
    lines.append('![pacing](%s)' % figures['pacing'])
    lines.append('')
    lines.append('Observed inter-send jitter (std/mean) per (case, '
                 'direction, transport). For uniform +/-j dither the ratio '
                 'tends to j/sqrt(3) (~0.029 at the harness default 0.01); '
                 'an exact metronome scores ~0. Systematic departures mean '
                 'the configured dither did not reach the publisher.')
    lines.append('')
    lines.append(markdown_table(pacing_summary))
    lines.append('')
    with open(os.path.join(output_dir, 'report.md'), 'w') as handle:
        handle.write('\n'.join(lines))


def parse_csv_list(value):
    """Split a comma-separated option into stripped non-empty items."""
    return [item.strip() for item in value.split(',') if item.strip()]


def build_parser():
    """Build the command-line parser for the analysis pipeline."""
    import argparse
    parser = argparse.ArgumentParser(
        description='Analyze raw benchmark CSVs into tables, figures, '
                    'and a Markdown report.')
    parser.add_argument('results_dir',
                        help='Directory containing raw CSV files')
    parser.add_argument('--output-dir', default=None,
                        help='Output dir (default: {results_dir}/analysis)')
    parser.add_argument('--cases', default=None,
                        help='Comma-separated case filter, e.g. '
                             'exp_constrained_pub_sub_xcdr (default: all)')
    parser.add_argument('--transports', default=None,
                        help='Comma-separated transport filter '
                             '(default: all)')
    parser.add_argument('--directions', default=None,
                        help='Comma-separated direction filter '
                             '(default: all)')
    parser.add_argument('--exclude-payloads', default='40M',
                        help='Comma-separated payload labels to exclude '
                             '(default: 40M)')
    parser.add_argument('--time-series-steps', default=None,
                        help='Comma-separated steps ("40K@100Hz"); default: '
                             'one step per payload at 100 Hz plus one per '
                             'frequency at 40K')
    parser.add_argument('--format', choices=('png', 'pdf'), default='png',
                        help='Figure output format (default: png)')
    parser.add_argument('--expected-jitter', type=float, default=0.0,
                        help='Expected uniform publish dither fraction for '
                             'the pacing reference overlay (default: 0 = '
                             'no overlay)')
    parser.add_argument('--dpi', type=int, default=150,
                        help='Figure DPI (default: 150)')
    parser.add_argument('--no-figures', action='store_true',
                        help='Skip figure generation (CSV only)')
    parser.add_argument('--no-report', action='store_true',
                        help='Skip markdown report generation')
    return parser


def run_pipeline(args):
    """Run the full load/join/tables/figures/report pipeline."""
    output_dir = args.output_dir or os.path.join(args.results_dir,
                                                 'analysis')
    os.makedirs(output_dir, exist_ok=True)
    df = load_raw_csvs(args.results_dir, skip_dirs=(output_dir,))
    if args.cases:
        keep_cases = set(parse_csv_list(args.cases))
        case = df['message'] + '_' + df['config'] + '_' + df['backend']
        df = df.loc[case.isin(keep_cases)].copy()
    if args.transports:
        df = df.loc[df['transport'].isin(
            parse_csv_list(args.transports))].copy()
    if args.directions:
        df = df.loc[df['direction'].isin(
            parse_csv_list(args.directions))].copy()
    exclude = set(parse_csv_list(args.exclude_payloads or ''))
    if exclude:
        df = df.loc[~df['payload_bytes'].isin(
            [parse_payload_label(p) for p in exclude])].copy()
    if df.empty:
        raise ValueError('no rows left after filtering')
    joined, n_publish, n_receive = join_pub_sub(df)
    metrics = derive_metrics(joined)

    per_run = per_run_stats(metrics, n_publish, n_receive)
    by_payload, by_frequency, by_direction = aggregate_sweeps(metrics)
    matrix = summary_matrix(metrics)
    drops = drops_table(per_run)
    pacing = pacing_table(metrics)
    pacing_summary = pacing_summary_table(pacing)
    per_run.to_csv(os.path.join(output_dir, 'per_run_stats.csv'),
                   index=False)
    by_payload.to_csv(os.path.join(output_dir, 'sweep_by_payload.csv'),
                      index=False)
    by_frequency.to_csv(
        os.path.join(output_dir, 'sweep_by_frequency.csv'), index=False)
    by_direction.to_csv(
        os.path.join(output_dir, 'sweep_by_direction.csv'), index=False)
    matrix.to_csv(os.path.join(output_dir, 'summary_matrix.csv'),
                  index=False)
    drops.to_csv(os.path.join(output_dir, 'drops.csv'), index=False)
    pacing.to_csv(os.path.join(output_dir, 'pacing.csv'), index=False)

    figures = {}
    fmt = args.format
    if not args.no_figures:
        pace = 'fig_pacing.%s' % fmt
        plot_pacing(pacing, args.expected_jitter,
                    os.path.join(output_dir, pace), dpi=args.dpi)
        figures['pacing'] = pace
        heat = 'fig_summary_heatmap.%s' % fmt
        plot_summary_heatmap(
            matrix, os.path.join(output_dir, heat), dpi=args.dpi)
        figures['heatmap'] = heat
        for case in sorted(matrix['case'].unique()):
            payloads = sorted(by_payload.loc[
                by_payload['case'] == case, 'payload_bytes'].unique())
            name = 'fig_sweep_payload_%s.%s' % (case, fmt)
            plot_sweep(by_payload, case, 'payload_bytes', 'payload_bytes',
                       payloads, os.path.join(output_dir, name),
                       dpi=args.dpi)
            figures['payload_%s' % case] = name
            freqs = sorted(by_frequency.loc[
                by_frequency['case'] == case,
                'target_frequency_hz'].unique())
            name = 'fig_sweep_freq_%s.%s' % (case, fmt)
            plot_sweep(by_frequency, case, 'target_frequency_hz',
                       'target_frequency_hz', freqs,
                       os.path.join(output_dir, name), dpi=args.dpi)
            figures['frequency_%s' % case] = name
        if args.time_series_steps:
            steps = parse_csv_list(args.time_series_steps)
        else:
            steps = default_time_series_steps(
                metrics['payload_bytes_pub'].unique(),
                metrics['target_frequency_hz_pub'].unique(),
                exclude_payloads=[parse_payload_label(p) for p in exclude
                                  if p])
        steps = list(dict.fromkeys(steps))
        kept_steps = []
        for label in steps:
            payload, freq = parse_step_label(label)
            present = metrics.loc[
                (metrics['payload_bytes_pub'] == payload) &
                (np.isclose(metrics['target_frequency_hz_pub'], freq))]
            if present.empty:
                sys.stderr.write('time-series step %s: no data, skipped\n'
                                 % label)
                continue
            name = 'fig_timeseries_%s.%s' % (
                label.replace('@', '_').replace('.', 'p'), fmt)
            plot_time_series(metrics, payload, freq,
                             os.path.join(output_dir, name), dpi=args.dpi)
            figures[label] = name
            kept_steps.append(label)
        steps = kept_steps
    else:
        steps = []
    if not args.no_report:
        write_report(output_dir, matrix, per_run, drops, steps, figures,
                     pacing_summary, image_format=fmt)
    return 0


def main(argv=None):
    """Parse arguments and run the pipeline."""
    parser = build_parser()
    return run_pipeline(parser.parse_args(argv))


if __name__ == '__main__':
    sys.exit(main())
