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

Streaming per-run design: each raw CSV is processed independently (never
more than one file's sample-level rows in memory) and per-run statistics
are accumulated into small tables; every figure is then rendered from
those tables alone.

Each run (one case x direction x transport x reliability x payload x
frequency) is the meaningful unit. No output combines sample-level data
across different runs: sweep lines connect per-run points, heatmap cells
hold one run's p50 / achieved_hz, and time-series / box plots overlay
one line / box per run in a meaningful set (exactly one parameter
varies, the rest are fixed at representative values).
"""

import os
import sys
import gc

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


def iter_csv_paths(results_dir, skip_dirs=()):
    """
    List every *.csv path under results_dir (minus skip_dirs), sorted.

    Pure path enumeration: no file contents are read, so this is O(files)
    in memory regardless of dataset size.
    """
    skip_dirs = {os.path.realpath(d) for d in skip_dirs}
    paths = []
    for root, _dirs, files in os.walk(results_dir):
        if os.path.realpath(root) in skip_dirs:
            continue
        for name in sorted(files):
            if not name.endswith('.csv'):
                continue
            paths.append(os.path.join(root, name))
    return paths


def load_single_csv(path):
    """
    Load one raw CSV file into a DataFrame, or None if skippable.

    Parses run_id into typed RUN_ID_PARTS columns and drops warmup plus
    error rows (explicit failures and undecodable foreign traffic, which
    carry sequence 0). Legit sequence-0 publish/receive samples are kept:
    the filter keys on the event column, not the sequence number. Files
    without the raw-run columns (analysis outputs, strays) and empty
    files are skipped with a stderr note; malformed lines are skipped by
    the parser. Peak memory is bounded by this one file.
    """
    try:
        frame = pd.read_csv(path, on_bad_lines='skip')
    except pd.errors.EmptyDataError:
        sys.stderr.write('skipping empty file: %s\n' % path)
        return None
    if not REQUIRED_COLUMNS.issubset(frame.columns):
        sys.stderr.write('skipping non-run file: %s\n' % path)
        return None
    if frame.empty:
        return None
    df = frame.loc[
        ~frame['run_id'].astype(str).str.startswith('run_id')].copy()
    if df.empty:
        return None
    try:
        # Parse each distinct run_id once (a file holds ~1e6 rows but only
        # ~1e2 distinct run_ids); per-row parse would re-parse the same
        # strings millions of times.
        run_id_str = df['run_id'].astype(str)
        uniq_ids = run_id_str.unique()
        parsed_unique = {rid: parse_run_id(rid) for rid in uniq_ids}
    except ValueError as exc:
        sys.stderr.write('skipping file with bad run_id %s: %s\n'
                         % (path, exc))
        return None
    for column in RUN_ID_PARTS:
        col_map = {rid: parsed_unique[rid][column] for rid in uniq_ids}
        df[column] = run_id_str.map(col_map)
    df['payload_bytes'] = df['payload_bytes'].astype(np.int64)
    df['sequence'] = df['sequence'].astype(np.int64)
    keep = (df['phase'] != 'warmup') & (df['event'] != 'error')
    df = df.loc[keep].reset_index(drop=True)
    if df.empty:
        return None
    return df


def load_raw_csvs(results_dir, skip_dirs=()):
    """
    Load every raw CSV under results_dir into one DataFrame.

    Compatibility shim for small inputs (unit tests): streams one file at
    a time via load_single_csv and concatenates. The production pipeline
    (run_pipeline) never calls this on large data; it uses
    collect_per_run_tables instead, which never holds more than one
    file's rows in memory.
    """
    frames = []
    for path in iter_csv_paths(results_dir, skip_dirs=skip_dirs):
        df = load_single_csv(path)
        if df is not None:
            frames.append(df)
    if not frames:
        raise ValueError('no CSV files under %r' % (results_dir,))
    return pd.concat(frames, ignore_index=True)


def apply_row_filters(df, cases=None, transports=None, directions=None,
                      exclude_payload_bytes=()):
    """
    Apply CLI row filters to one file's DataFrame (returns a copy).

    cases filters on the derived 'case' triple; the rest filter on the
    parsed run_id coordinates. exclude_payload_bytes drops whole payload
    sizes. Empty results return an empty frame (caller skips). A single
    combined mask means one copy regardless of how many filters apply.
    """
    mask = pd.Series(True, index=df.index)
    if cases:
        case = df['message'] + '_' + df['config'] + '_' + df['backend']
        mask &= case.isin(set(cases))
    if transports:
        mask &= df['transport'].isin(set(transports))
    if directions:
        mask &= df['direction'].isin(set(directions))
    if exclude_payload_bytes:
        mask &= ~df['payload_bytes'].isin(set(exclude_payload_bytes))
    return df.loc[mask].copy()


#: Log-spaced latency histogram edges (µs) shared by every pooled group:
#: 512 bins from 1µs to 300s (~3.7% per bin) resolve p50/p95/p99 while
#: keeping all groups' accumulators under a megabyte combined.
POOLED_LAT_EDGES = np.logspace(np.log10(1.0), np.log10(3e8), 513)

#: Reservoir capacity per (transport, reliability) box-plot pool: a uniform
#: random subsample this large renders distribution shape faithfully.
POOLED_RESERVOIR = 4096

#: Reservoir capacity per grid cell (one payload x frequency corner of one
#: run-kind): boxes need quartiles, not full shapes, so a small uniform
#: subsample suffices. 180 kinds x 42 cells x 512 floats stays ~30MB.
CELL_RESERVOIR = 512

#: Regime corner coordinates: 4KB/4MB payloads crossed with 10/1000Hz,
#: snapped to the figure-visible grid. 40B is header-dominated and 40M
#: is the saturated corner told by the throughput charts; 4KB is a
#: typical small message, 4MB a large one short of saturation.
CORNER_SMALL_PAYLOAD = 4000
CORNER_LARGE_PAYLOAD = 4000000
CORNER_LOW_FREQ = 10.0
CORNER_HIGH_FREQ = 1000.0


def _snap_to_grid(want, options):
    """Snap a wanted coordinate to the nearest available grid value."""
    return min(options, key=lambda v: abs(v - want))


def new_pooled():
    """Create empty pooled-latency accumulators for one collection pass."""
    return {'groups': {}, 'cells': {}}


def _reservoir_update(state, values, rng, capacity=POOLED_RESERVOIR):
    """
    Merge a batch of samples into a uniform reservoir (vectorized).

    state holds 'res' (kept values) and 'seen' (total items offered so
    far). Algorithm R: the first `capacity` items fill the reservoir,
    item k>capacity replaces a uniform random slot with probability
    capacity/k. Deterministic for a fixed rng sequence and file order.
    """
    vals = np.asarray(values, dtype=np.float64)
    vals = vals[np.isfinite(vals) & (vals > 0)]
    if len(vals) == 0:
        return
    res = state['res']
    n0 = state['seen']
    m = len(vals)
    if n0 + m <= capacity:
        state['res'] = np.concatenate([res, vals])
        state['seen'] = n0 + m
        return
    fill = capacity - len(res)
    pos = 0
    if fill > 0:
        res = np.concatenate([res, vals[:fill]])
        pos = fill
    rest = m - pos
    if rest > 0:
        k = np.arange(n0 + pos + 1, n0 + m + 1, dtype=np.float64)
        keep = rng.random(rest) < (capacity / k)
        n_keep = int(keep.sum())
        if n_keep:
            slots = rng.integers(0, capacity, size=n_keep)
            res = res.copy()
            res[slots] = vals[pos:][keep]
    state['res'] = res
    state['seen'] = n0 + m


def accumulate_pooled(pooled, metrics, rng):
    """
    Fold one file's joined samples into pooled latency accumulators.

    Per (transport, reliability, case, direction) group: sample count, a
    log-histogram for pooled p50/p95/p99, and a uniform reservoir for
    box plots. All payloads and frequencies pool together within a
    group; groups are never mixed, so cross-transport comparison stays
    apples-to-apples over the identical sweep grid. Additionally, one
    small uniform reservoir per grid cell (group x payload x frequency)
    feeds the per-corner regime boxes, so load regimes are never mixed
    there either. Figure exclusions are applied when reading the
    accumulators, not while collecting.

    Implementation note: the per-file coords DataFrame plus groupby is
    deliberate (consistent with the join/derive frames around it). It is
    bounded at one file's rows and negligible next to CSV parsing and
    the pub/sub join, so no dict fast-path.
    """
    lat_all = metrics['latency_us'].to_numpy(dtype=np.float64)
    coords = pd.DataFrame({
        'transport': metrics['transport_pub'].astype(str),
        'reliability': metrics['reliability_pub'].astype(str),
        'case': (metrics['message_pub'].astype(str) + '_' +
                 metrics['config_pub'].astype(str) + '_' +
                 metrics['backend_pub'].astype(str)),
        'direction': metrics['direction_pub'].astype(str),
        'payload': metrics['payload_bytes_pub'].to_numpy(dtype=np.int64),
        'freq': metrics['target_frequency_hz_pub'].to_numpy(
            dtype=np.float64),
    })
    edges = POOLED_LAT_EDGES
    nbins = len(edges) - 1
    grouped = coords.groupby(
        ['transport', 'reliability', 'case', 'direction'], observed=True,
        sort=True)
    for key, row_idx in grouped.indices.items():
        # Row order within a group is irrelevant: histograms commute and
        # reservoir uniformity holds for any arrival order.
        _accumulate_block(pooled, key, lat_all[np.asarray(row_idx)],
                          rng, edges, nbins)
    cell_grouped = coords.groupby(
        ['transport', 'reliability', 'case', 'direction', 'payload',
         'freq'],
        observed=True, sort=True)
    for key, row_idx in cell_grouped.indices.items():
        _accumulate_cell(pooled, key, lat_all[np.asarray(row_idx)], rng)


def _accumulate_cell(pooled, key, lat, rng):
    """Fold one grid-cell sample block into its small reservoir."""
    vals = lat[np.isfinite(lat) & (lat > 0)]
    if len(vals) == 0:
        return
    entry = pooled['cells'].get(key)
    if entry is None:
        entry = {'res': np.empty(0, dtype=np.float64), 'seen': 0}
        pooled['cells'][key] = entry
    _reservoir_update(entry, vals, rng, capacity=CELL_RESERVOIR)


def _accumulate_block(pooled, key, lat, rng, edges, nbins):
    """Fold one contiguous run-group sample block into the accumulators."""
    vals = lat[np.isfinite(lat) & (lat > 0)]
    if len(vals) == 0:
        return
    entry = pooled['groups'].get(key)
    if entry is None:
        entry = {'n': 0, 'hist': np.zeros(nbins, dtype=np.int64),
                 'res': np.empty(0, dtype=np.float64), 'seen': 0}
        pooled['groups'][key] = entry
    entry['n'] += len(vals)
    idx = np.searchsorted(edges, vals, side='right') - 1
    np.clip(idx, 0, nbins - 1, out=idx)
    entry['hist'] += np.bincount(idx, minlength=nbins)
    _reservoir_update(entry, vals, rng)


def pooled_quantile(edges, counts, q):
    """
    Invert a log-histogram at probability q (geometric interpolation).

    Returns NaN for empty accumulators. Bin resolution (~3.7%) bounds
    the error; exact per-run quantiles live in per_run_stats.
    """
    total = float(np.sum(counts))
    if total == 0:
        return float('nan')
    target = min(max(q * total, 1.0), total)
    cum = np.cumsum(counts)
    i = int(np.searchsorted(cum, target, side='left'))
    i = min(max(i, 0), len(counts) - 1)
    prev = float(cum[i - 1]) if i > 0 else 0.0
    frac = (target - prev) / float(counts[i]) if counts[i] else 0.0
    lo, hi = float(edges[i]), float(edges[i + 1])
    return lo * (hi / lo) ** min(max(frac, 0.0), 1.0)


def collect_per_run_tables(results_dir, skip_dirs=(), cases=None,
                           transports=None, directions=None,
                           exclude_payload_bytes=()):
    """
    Stream all raw CSVs into per-run statistics (bounded memory).

    Processes one file at a time: load, filter, join pub/sub, derive
    metrics, compute per-run stats plus pacing rows for the runs in that
    file, and fold the samples into pooled latency accumulators (shared
    log-histograms per run-kind plus uniform reservoirs per transport
    pair for boxes), then discards the sample-level frame. Assumes
    each run_id lives in exactly one file (true for the matrix driver,
    whose invocations cover disjoint case x direction x reliability
    slices).

    Returns (per_run, pacing, pooled): small per-run DataFrames plus the
    pooled accumulators (kilobytes). Raises ValueError when no rows
    survive filtering.
    """
    per_run_frames = []
    pacing_frames = []
    pooled = new_pooled()
    rng = np.random.default_rng(20260916)
    n_files = 0
    for path in iter_csv_paths(results_dir, skip_dirs=skip_dirs):
        df = load_single_csv(path)
        if df is None:
            continue
        df = apply_row_filters(
            df, cases=cases, transports=transports,
            directions=directions,
            exclude_payload_bytes=exclude_payload_bytes)
        if df.empty:
            continue
        n_files += 1
        joined, n_publish, n_receive = join_pub_sub(df)
        del df
        if joined.empty:
            continue
        metrics = derive_metrics(joined)
        del joined
        per_run_frames.append(per_run_stats(metrics, n_publish, n_receive))
        pacing_frames.append(pacing_table(metrics))
        accumulate_pooled(pooled, metrics, rng)
        del metrics
        # del only drops the reference: CPython refcounting leaves pandas
        # block-manager cycles behind, and without an explicit collection
        # the per-file ~400MB frames accumulate across the 60-file matrix
        # until the process OOMs. Forcing a cycle collection here keeps
        # peak memory bounded at one file plus the small accumulators.
        gc.collect()
    if not per_run_frames:
        raise ValueError('no rows left after filtering')
    per_run = pd.concat(per_run_frames, ignore_index=True)
    pacing = pd.concat(pacing_frames, ignore_index=True)
    sys.stderr.write('streamed %d files -> %d runs\n'
                     % (n_files, len(per_run)))
    return per_run, pacing, pooled


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
    drop_rate (1 - n_receive / n_publish from pre-join counts), n_late
    (joined samples slower than one publish period), and missed_frac
    (1 - on-time receives / publishes: late plus dropped over everything
    published). Sweep coordinates ride along from the first sample.
    """
    grouped = metrics.groupby('run_id', observed=True)
    stats = grouped['latency_us'].agg(
        n='count', mean='mean', std='std', min='min', max='max',
        p50=lambda s: s.quantile(0.50), p95=lambda s: s.quantile(0.95),
        p99=lambda s: s.quantile(0.99))
    stats = stats.reset_index()
    lat = metrics['latency_us'].to_numpy(dtype=np.float64)
    freq = metrics['target_frequency_hz_pub'].to_numpy(dtype=np.float64)
    with np.errstate(divide='ignore', invalid='ignore'):
        period_us = np.where(freq > 0, 1e6 / freq, np.inf)
    late = (lat > period_us).astype(np.int64)
    n_late = pd.Series(late, index=metrics['run_id']).groupby(
        level=0, observed=True).sum()
    stats = pd.merge(stats, n_late.rename('n_late'),
                     left_on='run_id', right_index=True, how='left')
    stats['n_late'] = stats['n_late'].fillna(0).astype(np.int64)
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
    stats['missed_frac'] = (
        1.0 - (stats['n'] - stats['n_late']) / stats['n_publish'])
    stats['case'] = (
        stats['message'] + '_' + stats['config'] + '_' + stats['backend'])
    return stats


#: Default representative case/direction for transport-comparison and
#: saturation figures (one of several fixed coordinates; payload and
#: frequency defaults resolve to dataset values at runtime).
DEFAULT_TS_CASE = 'exp_constrained_pub_only_xcdr'
DEFAULT_TS_DIRECTION = 'cpp_to_py'


def median_of(values):
    """Return the median of a value list as float/int (NaN when empty)."""
    arr = np.asarray(list(values), dtype=np.float64)
    arr = arr[np.isfinite(arr)]
    if arr.size == 0:
        return float('nan')
    return float(np.median(arr))


def resolve_default_case(per_run, requested=None):
    """
    Resolve the default case for fixed-value plots.

    Prefers the requested string when present; else prefers any case
    containing 'constrained_pub_only'; else the first sorted case.
    """
    cases = sorted(per_run['case'].unique())
    if requested and requested in set(cases):
        return requested
    for cand in cases:
        if 'constrained_pub_only' in cand:
            return cand
    for cand in cases:
        if requested and requested in cand:
            return cand
    return cases[0]


def parse_exclude_list(value, default=()):
    """Parse a comma-separated exclusion list (empty/None -> default)."""
    if value is None:
        return list(default)
    items = [item.strip() for item in str(value).split(',') if item.strip()]
    return items if items or value == '' else list(default)


def resolve_fixed_values(per_run, args):
    """
    Resolve all fixed representative values for plots from CLI + data.

    Figure exclusions are resolved first; every fixed payload then snaps
    to the figure-visible set (never to an excluded payload — a fixed
    value outside the figure set would render every figure of its kind
    empty). Throughput defaults to the largest figure-visible payload.
    """
    figure_exclude_cases = parse_exclude_list(
        getattr(args, 'figure_exclude_cases', None),
        DEFAULT_FIGURE_EXCLUDE_CASES)
    figure_exclude_directions = parse_exclude_list(
        getattr(args, 'figure_exclude_directions', None),
        DEFAULT_FIGURE_EXCLUDE_DIRECTIONS)
    if getattr(args, 'figure_exclude_cases', None) == '':
        figure_exclude_cases = []
    if getattr(args, 'figure_exclude_directions', None) == '':
        figure_exclude_directions = []
    figure_exclude_payloads = [
        parse_payload_label(p)
        for p in parse_csv_list(getattr(args, 'exclude_payloads', None)
                                or '')]
    fig_payloads = sorted(
        set(int(p) for p in per_run['payload_bytes'].unique())
        - set(figure_exclude_payloads))
    if not fig_payloads:
        fig_payloads = sorted(
            set(int(p) for p in per_run['payload_bytes'].unique()))
    obs_freqs = sorted(float(f) for f in
                       per_run['target_frequency_hz'].unique())
    raw_med_payload = median_of(fig_payloads)
    raw_med_freq = median_of(obs_freqs)
    med_payload = min(fig_payloads,
                      key=lambda p: abs(p - raw_med_payload))
    med_freq = min(obs_freqs, key=lambda f: abs(f - raw_med_freq))
    sweep_freq = med_freq
    if getattr(args, 'sweep_fixed_freq', None):
        try:
            sweep_freq = parse_freq_label(args.sweep_fixed_freq)
        except ValueError:
            sweep_freq = med_freq
    if not any(np.isclose(f, sweep_freq) for f in obs_freqs):
        sweep_freq = min(obs_freqs, key=lambda f: abs(f - sweep_freq))
    sweep_payload = med_payload
    if getattr(args, 'sweep_fixed_payload', None):
        try:
            sweep_payload = parse_payload_label(args.sweep_fixed_payload)
        except ValueError:
            sweep_payload = med_payload
    if sweep_payload not in set(fig_payloads):
        sweep_payload = min(fig_payloads,
                            key=lambda p: abs(p - sweep_payload))
    return {
        'sweep_fixed_freq': float(sweep_freq),
        'sweep_fixed_payload': int(sweep_payload),
        'ts_case': resolve_default_case(
            per_run, getattr(args, 'ts_case', None) or DEFAULT_TS_CASE),
        'ts_direction': (getattr(args, 'ts_direction', None)
                         or DEFAULT_TS_DIRECTION),
        'figure_exclude_cases': figure_exclude_cases,
        'figure_exclude_directions': figure_exclude_directions,
        'figure_exclude_payloads': figure_exclude_payloads,
        'line_exclude_cases': parse_exclude_list(
            getattr(args, 'line_exclude_cases', None),
            DEFAULT_LINE_EXCLUDE_CASES),
        'throughput_xmin_bps': _parse_xmin(
            getattr(args, 'throughput_xmin', None)),
    }


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


#: Transport colors (IBM palette) and symbols, used identically in
#: every figure. Symbol order runs square, circle, triangle
#: (decreasing area) so overlapping markers stay readable.
TRANSPORT_COLORS = {
    'udp': '#648FFF',
    'shmem': '#FFB000',
    'shmem_ds': '#785EF0',
}
TRANSPORT_MARKERS = {
    'udp': 's',
    'shmem': 'o',
    'shmem_ds': '^',
}
#: Canonical transport order for iteration and legends.
TRANSPORT_ORDER = ('udp', 'shmem', 'shmem_ds')

#: Machine memory ceilings (bytes/sec) for the benchmark host: DDR4
#: single-channel 64-bit at 3200MT/s is 25.6GB/s theoretical; measured
#: STREAM copy 22.3GB/s, memset 19.14GB/s and memcpy 11.74GB/s on 40M
#: blocks. The memcpy line is the binding constraint for copy paths
#: (filling is a memset); achieved maxima hug it.
MEMCPY_CEILING_BPS = 11.74e9
MEMSET_CEILING_BPS = 19.14e9
STREAM_CEILING_BPS = 22.3e9

#: Memory ceilings as (bytes/sec, short name, gray shade, line type) for
#: throughput figures: dashed memcpy, dotted memset, dash-dot STREAM.
#: Labeled once in a centered bottom caption, never in legends.
MEMORY_CEILINGS = (
    (MEMCPY_CEILING_BPS, 'memcpy ceiling', '#777777', '--'),
    (MEMSET_CEILING_BPS, 'memset ceiling', '#555555', ':'),
    (STREAM_CEILING_BPS, 'STREAM ceiling', '#333333', '-.'),
)


def ceiling_subtitle(fig, y=0.92):
    """
    Subtitle under the figure title naming the ceiling line types.
    """
    glyph = {'--': '- -', ':': '.....', '-.': '-.-.'}
    parts = ['%s %s %s' % (glyph.get(style, '--'), name,
                           format_throughput_bps(value))
             for value, name, _shade, style in MEMORY_CEILINGS]
    fig.text(0.5, y, 'Ceilings, left to right: ' + '    '.join(parts),
             ha='center', fontsize=12, color='#555555')

#: Canonical direction order for subplot grids.
DIRECTIONS = (
    'cpp_to_cpp', 'cpp_to_py', 'py_to_cpp',
    'py_to_py', 'intra_cpp', 'intra_py',
)

#: Cases excluded from figures by default (kept in CSVs): experimental
#: copy runs are within expectation and add no narrative.
DEFAULT_FIGURE_EXCLUDE_CASES = ('exp_copy_xcdr',)

#: Directions excluded from figures by default (kept in CSVs): none. The
#: intra-process +1-period artifact (single-executor sleep) is fixed and
#: intra_py never had it, so intra runs are first-class figure citizens.
#: Pass --figure-exclude-directions to hide any direction again.
DEFAULT_FIGURE_EXCLUDE_DIRECTIONS = ()

#: Cases hidden from connected-line figures and distribution boxes (kept
#: in tables, pacing, and CSVs): Copy XCDR tracks Copy FastCDR within
#: distribution and adds no narrative to line plots; boxes focus on
#: constrained/loaned XCDR runs against the FastCDR baseline.
DEFAULT_LINE_EXCLUDE_CASES = ('std_copy_xcdr',)


#: Set once by ensure_backend; matplotlib.use() after pyplot import is a
#: no-op at best, so every plot entry point goes through the guard.
_BACKEND_SET = False


def ensure_backend():
    """Select the headless matplotlib backend (one-shot, idempotent)."""
    global _BACKEND_SET
    if _BACKEND_SET:
        return
    import matplotlib
    matplotlib.use('Agg')
    # Figure text sizes shared by every plot entry point: suptitles read
    # as titles (15), tick labels match axis labels (12). Legends and
    # axis captions carry explicit fontsize=12 at their call sites.
    matplotlib.rcParams.update({
        'xtick.labelsize': 12,
        'ytick.labelsize': 12,
        'figure.titlesize': 15,
    })
    _BACKEND_SET = True


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


def format_payload_si(payload_bytes):
    """
    Format any payload size with SI suffixes for axis ticks.

    Unlike format_payload (which preserves run-id label round-tripping),
    this always renders units: 400 -> '400B', 1000 -> '1K', 1000000 ->
    '1M', 1000000000 -> '1G'. Never scientific notation.
    """
    payload_bytes = float(payload_bytes)
    if not np.isfinite(payload_bytes):
        return 'n/a'
    if payload_bytes >= 1000000000:
        return '%gG' % (payload_bytes / 1000000000.0)
    if payload_bytes >= 1000000:
        return '%gM' % (payload_bytes / 1000000.0)
    if payload_bytes >= 1000:
        return '%gK' % (payload_bytes / 1000.0)
    return '%gB' % payload_bytes


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


def format_throughput_bps(bps):
    """Format bytes/sec with SI suffixes (40000 -> '40KB/s')."""
    if not np.isfinite(bps):
        return 'n/a'
    if bps >= 1000000000000:
        return '%gTB/s' % (bps / 1000000000000.0)
    if bps >= 1000000000:
        return '%gGB/s' % (bps / 1000000000.0)
    if bps >= 1000000:
        return '%gMB/s' % (bps / 1000000.0)
    if bps >= 1000:
        return '%gKB/s' % (bps / 1000.0)
    return '%gB/s' % bps


def format_freq_pretty(hz):
    """Format a frequency for axes ('10 Hz', '2.15 Hz')."""
    if not np.isfinite(hz):
        return 'n/a'
    return '%.6g Hz' % hz


def format_freq_si(hz):
    """
    Format a frequency with SI suffixes for axis ticks (1000 -> '1 kHz').

    Unlike format_freq_pretty (which preserves run-id label round-tripping
    for titles and messages), this always renders units, so kilohertz
    grid values read '1 kHz' instead of '1000 Hz'. Never scientific
    notation.
    """
    if not np.isfinite(hz):
        return 'n/a'
    if hz >= 1000:
        return '%g kHz' % (hz / 1000.0)
    return '%.6g Hz' % hz


def format_ms(value):
    """Format a millisecond duration with SI units ('10ms', '1.5s')."""
    if not np.isfinite(value):
        return 'n/a'
    if value < 1.0:
        return '%gµs' % (value * 1000.0)
    if value < 1000.0:
        return '%gms' % value
    return '%gs' % (value / 1000.0)


def humanize_case(case):
    """
    Spell out a case triple for legends and titles.

    Backend + data path carry the narrative (FastCDR vs XCDR, copy vs
    constrained); the std/exp message prefix is dropped except where two
    cases would otherwise collide (std_copy_xcdr vs exp_copy_xcdr).
    """
    mapping = {
        'std_copy_fastcdr': 'Copy FastCDR',
        'std_copy_xcdr': 'Copy XCDR',
        'exp_copy_xcdr': 'Copy XCDR (exp message)',
        'exp_constrained_pub_only_xcdr': 'Constrained publisher XCDR',
        'exp_constrained_pub_sub_xcdr': 'Constrained pub/sub XCDR',
    }
    return mapping.get(case, case)


def humanize_direction(direction):
    """Spell out a direction for legends and titles."""
    mapping = {
        'cpp_to_cpp': 'C++ to C++',
        'cpp_to_py': 'C++ to Python',
        'py_to_cpp': 'Python to C++',
        'py_to_py': 'Python to Python',
        'intra_cpp': 'Intra-process C++',
        'intra_py': 'Intra-process Python',
    }
    return mapping.get(direction, direction)


def humanize_transport(transport):
    """Spell out a transport for legends and titles."""
    mapping = {
        'udp': 'UDP',
        'shmem': 'Shared memory',
        'shmem_ds': 'Shared memory + data sharing',
    }
    return mapping.get(transport, transport)


def humanize_reliability(reliability):
    """Spell out a reliability level for legends and titles."""
    mapping = {
        'reliable': 'Reliable',
        'best_effort': 'Best effort',
    }
    return mapping.get(reliability, reliability)


#: Backend spellings for descriptive run-kind labels.
BACKEND_LABELS = {
    'fastcdr': 'FastCDR',
    'xcdr': 'XCDR',
}

#: Data-path spellings by config for descriptive run-kind labels:
#: plain copy, loaned publisher against bounded writes, loaned both ends.
CONFIG_MODES = {
    'copy': 'copy',
    'constrained_pub_only': 'loan (write bounded)',
    'constrained_pub_sub': 'loan (bounded)',
}


def descriptive_label(case, direction):
    """
    Direction-first run-kind label decomposing the case triple.

    E.g. 'C++ to Python, XCDR, loan (write bounded)': fully spelled out
    direction, backend, and data path. The std/exp message prefix is
    dropped, so std and exp copies of one backend share a label (listed
    as separate legend entries on one shared line handle, never merged).
    """
    fields = case.split('_')
    if len(fields) >= 3:
        backend = BACKEND_LABELS.get(fields[-1], fields[-1])
        mode = CONFIG_MODES.get('_'.join(fields[1:-1]),
                                '_'.join(fields[1:-1]))
    else:
        backend, mode = case, ''
    label = '%s, %s' % (humanize_direction(direction), backend)
    return '%s, %s' % (label, mode) if mode else label


def pair_label(transport, reliability):
    """Fully spelled out label for one (transport, reliability) pair."""
    return '%s, %s' % (humanize_transport(transport),
                       humanize_reliability(reliability))


def axis_formatter(kind):
    """Return a matplotlib FuncFormatter rendering SI units for kind."""
    def _fmt(value, _pos):
        if kind == 'latency_us':
            return format_us(float(value))
        if kind == 'payload_bytes':
            return format_payload_si(float(value))
        if kind == 'frequency_hz':
            return format_freq_si(float(value))
        if kind == 'throughput_bps':
            return format_throughput_bps(float(value))
        if kind == 'milliseconds':
            return format_ms(float(value))
        return '%.6g' % value
    return _fmt


def apply_log_ticks(ax, x_kind=None, y_kind=None, x_ticks=None,
                    subs=(1.0, 2.0, 5.0)):
    """
    SI log axes with readable tick density, never scientific notation.

    Both axes keep log scale; subdivisions per decade (default 1/2/5) so
    a typical latency span shows half a dozen labeled ticks instead of
    two, or plain decades with subs=None for spans covering many
    decades. x_ticks pins labels to observed data values (e.g. the six
    grid frequencies) instead of bare decades. numticks is set high: matplotlib's subs locator otherwise
    returns zero ticks on wide spans.
    """
    from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter
    # subs=None means plain decades: matplotlib's default locator strides
    # oddly on wide spans (ticks outside the data, scientific extremes),
    # so decades are requested explicitly instead. Axes without a kind
    # (e.g. categorical box positions) are left untouched. Minor ticks
    # keep their locations (grid) but never labels: matplotlib's default
    # minor log formatter renders math notation (6x10^1), which is banned.
    if x_kind is not None:
        if x_ticks is not None:
            ax.set_xticks(sorted(set(float(v) for v in x_ticks)))
        else:
            x_subs = (1.0,) if subs is None else subs
            ax.xaxis.set_major_locator(
                LogLocator(subs=x_subs, numticks=100))
        ax.xaxis.set_major_formatter(FuncFormatter(axis_formatter(x_kind)))
        ax.xaxis.set_minor_formatter(NullFormatter())
    if y_kind is not None:
        y_subs = (1.0,) if subs is None else subs
        ax.yaxis.set_major_locator(
            LogLocator(subs=y_subs, numticks=100))
        ax.yaxis.set_major_formatter(FuncFormatter(axis_formatter(y_kind)))
        ax.yaxis.set_minor_formatter(NullFormatter())


def _span_subs(*arrays):
    """
    Choose log subdivisions from the plotted span.

    Wide spans (over five decades, e.g. unzoomed throughput) get plain
    decades so labels stay readable; narrower spans get 1/2/5
    subdivisions. Returns None for decades (see apply_log_ticks).
    """
    vals = np.concatenate(
        [np.asarray(a, dtype=np.float64) for a in arrays
         if len(np.asarray(a)) > 0])
    vals = vals[np.isfinite(vals) & (vals > 0)]
    if len(vals) == 0:
        return None
    span = float(np.log10(vals.max()) - np.log10(vals.min()))
    return None if span > 5 else (1.0, 2.0, 5.0)


def filter_figure_runs(per_run, exclude_cases=(),
                       exclude_directions=(), exclude_payloads=()):
    """
    Return per_run minus figure-excluded runs (copy).

    Cases, directions, and payloads are hidden from figures only; the
    per-run CSV tables always keep every run. Payload exclusion is a
    manual override (--exclude-payloads); sweeps draw every run as
    measured.
    """
    mask = pd.Series(True, index=per_run.index)
    if exclude_cases:
        mask = mask & (~per_run['case'].isin(set(exclude_cases)))
    if exclude_directions:
        mask = mask & (~per_run['direction'].isin(set(exclude_directions)))
    if exclude_payloads:
        mask = mask & (~per_run['payload_bytes'].isin(
            set(int(p) for p in exclude_payloads)))
    return per_run.loc[mask].copy()


def add_throughput_columns(per_run):
    """
    Add achieved/expected throughput in bytes/sec to a per-run table.

    achieved_throughput_Bps = achieved_hz * payload_bytes (average over the
    send window); expected_throughput_Bps = target_frequency_hz *
    payload_bytes. Returns a copy.
    """
    out = per_run.copy()
    out['achieved_throughput_Bps'] = (
        out['achieved_hz'].to_numpy(dtype=np.float64)
        * out['payload_bytes'].to_numpy(dtype=np.float64))
    out['expected_throughput_Bps'] = (
        out['target_frequency_hz'].to_numpy(dtype=np.float64)
        * out['payload_bytes'].to_numpy(dtype=np.float64))
    return out


def _series_styles(keys):
    """
    Assign a distinct color/marker per series key.

    Colors follow the IBM colorblind-friendly palette; symbols run
    square, circle, triangle, ... (decreasing area), so when lines
    overlap the smaller later-drawn markers stay visible on top of the
    larger earlier ones. Returns a dict key -> (color, marker).
    """
    colors = ('#648FFF', '#DC267F', '#FFB000', '#785EF0', '#FE6100')
    markers = ('s', 'o', '^', 'D', 'v', '<', '>', 'p', '*', 'h')
    styles = {}
    for i, key in enumerate(sorted(keys)):
        styles[key] = (colors[i % len(colors)],
                       markers[i % len(markers)])
    return styles


#: Short case identity for legends inside direction cells: direction
#: lives in the cell title, so the label carries backend + data path.
#: std and exp copies of one backend share a label, listed separately.
CASE_SHORT_LABELS = {
    'std_copy_fastcdr': 'FastCDR copy',
    'std_copy_xcdr': 'XCDR copy',
    'exp_copy_xcdr': 'XCDR copy',
    'exp_constrained_pub_only_xcdr': 'XCDR loan (write bounded)',
    'exp_constrained_pub_sub_xcdr': 'XCDR loan (bounded)',
}


def case_short_label(case):
    """Short case label for legends inside direction cells."""
    return CASE_SHORT_LABELS.get(case, humanize_case(case))


#: Direction cells for grid figures: one subplot per direction, lines
#: (or boxes) per case inside. intra_cpp reads with C++ to C++,
#: intra_py with Python to Python — but in their own cells, since
#: intra runners are not the same animals as inter-process pairs.
DIRECTION_CELLS = (
    ('C++ to C++', ('cpp_to_cpp',)),
    ('C++ to C++ (intra)', ('intra_cpp',)),
    ('C++ to Python', ('cpp_to_py',)),
    ('Python to C++', ('py_to_cpp',)),
    ('Python to Python', ('py_to_py',)),
    ('Python to Python (intra)', ('intra_py',)),
)


def _draw_sweep_cell(ax, cell, xcol, styles):
    """
    Draw one direction cell: one connected line per case.

    xcol selects the sweep axis ('payload_bytes' or
    'target_frequency_hz'); y is always per-run p50. Lines carry the
    short case label (direction lives in the cell title). Empty cells
    are left for the caller to annotate.
    """
    for case in sorted(set(cell['case'])):
        line = cell.loc[cell['case'] == case].sort_values(xcol)
        if line.empty:
            continue
        x = line[xcol].to_numpy(dtype=np.float64)
        y = line['p50'].to_numpy(dtype=np.float64)
        color, marker = styles[case]
        ax.plot(x, y, label=case_short_label(case), color=color,
                marker=marker, linewidth=2)


def plot_sweep_payload(per_run, transport, reliability, fixed_freq,
                       out_latency_path, dpi=150):
    """
    Write the payload-sweep latency figure for one (transport, reliability).

    Filters per_run to the pair plus fixed_freq, then draws a 2x3 grid
    of direction cells (intra-process directions get their own cells),
    one connected line per case with one point per run (no error bars),
    degraded runs included: latency balloons where delivery collapses.
    Axes use SI units with shared log scales; legend labels name the
    case (direction lives in the cell title).
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    subset = per_run.loc[
        (per_run['transport'] == transport) &
        (per_run['reliability'] == reliability) &
        (np.isclose(per_run['target_frequency_hz'].to_numpy(
            dtype=np.float64), float(fixed_freq)))]
    if subset.empty:
        sys.stderr.write('sweep payload %s %s @ %s: no data, skipped\n' % (
            transport, reliability, format_freq_pretty(float(fixed_freq))))
        return False
    cases = sorted(set(subset['case']))
    if not cases:
        return False
    styles = _series_styles(cases)
    fig, axes = plt.subplots(2, 3, figsize=(18, 10),
                             sharex=True, sharey=True)
    x_ticks = sorted(subset['payload_bytes'].unique())
    for ax, (group_label, directions) in zip(
            axes.flat, DIRECTION_CELLS):
        cell = subset.loc[subset['direction'].isin(set(directions))]
        ax.set_xscale('log')
        ax.set_yscale('log')
        if cell.empty:
            ax.text(0.5, 0.5, 'no data', ha='center', va='center',
                    transform=ax.transAxes)
        else:
            _draw_sweep_cell(ax, cell, 'payload_bytes', styles)
        ax.set_title(group_label)
        apply_log_ticks(ax, x_kind='payload_bytes', y_kind='latency_us',
                        x_ticks=x_ticks)
        ax.grid(True, which='both', alpha=0.3)
        handles, _labels = ax.get_legend_handles_labels()
        if handles:
            ax.legend(loc='upper left', fontsize=12)
    fig.suptitle('%s payload sweep at %s (p50 latency, per run)' % (
        pair_label(transport, reliability),
        format_freq_pretty(float(fixed_freq))))
    fig.text(0.5, 0.02, 'Payload size', ha='center', fontsize=12)
    fig.text(0.02, 0.5, 'p50 latency', va='center', rotation='vertical',
             fontsize=12)
    fig.tight_layout(rect=[0.04, 0.04, 1, 0.94])
    fig.savefig(out_latency_path, dpi=dpi)
    plt.close(fig)
    return True


def plot_sweep_frequency(per_run, transport, reliability, fixed_payload,
                         out_latency_path, dpi=150):
    """
    Write the frequency-sweep latency figure for one (transport, reliab.).

    Filters per_run to the pair plus fixed_payload, then draws a 2x3
    grid of direction cells (intra-process directions get their own
    cells), one connected line per case with one point per run,
    degraded runs included. SI units with shared log scales on both
    axes (kilohertz ticks read '1 kHz'); x tick labels are slanted to
    avoid overlap. Legend labels name the case.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    subset = per_run.loc[
        (per_run['transport'] == transport) &
        (per_run['reliability'] == reliability) &
        (per_run['payload_bytes'] == int(fixed_payload))]
    if subset.empty:
        sys.stderr.write('sweep frequency %s %s @ %s: no data, skipped\n'
                         % (transport, reliability,
                            format_payload(int(fixed_payload))))
        return False
    cases = sorted(set(subset['case']))
    if not cases:
        return False
    styles = _series_styles(cases)
    fig, axes = plt.subplots(2, 3, figsize=(18, 10),
                             sharex=True, sharey=True)
    x_ticks = sorted(subset['target_frequency_hz'].unique())
    for ax, (group_label, directions) in zip(
            axes.flat, DIRECTION_CELLS):
        cell = subset.loc[subset['direction'].isin(set(directions))]
        ax.set_xscale('log')
        ax.set_yscale('log')
        if cell.empty:
            ax.text(0.5, 0.5, 'no data', ha='center', va='center',
                    transform=ax.transAxes)
        else:
            _draw_sweep_cell(ax, cell, 'target_frequency_hz', styles)
        ax.set_title(group_label)
        apply_log_ticks(
            ax, x_kind='frequency_hz', y_kind='latency_us',
            x_ticks=x_ticks)
        plt.setp(ax.get_xticklabels(), rotation=30, ha='right')
        ax.grid(True, which='both', alpha=0.3)
        handles, _labels = ax.get_legend_handles_labels()
        if handles:
            ax.legend(loc='lower left', fontsize=12)
    fig.suptitle('%s frequency sweep at %s (p50 latency, per run)' % (
        pair_label(transport, reliability),
        format_payload(int(fixed_payload))))
    fig.text(0.5, 0.02, 'Publish frequency', ha='center', fontsize=12)
    fig.text(0.02, 0.5, 'p50 latency', va='center', rotation='vertical',
             fontsize=12)
    fig.tight_layout(rect=[0.04, 0.04, 1, 0.94])
    fig.savefig(out_latency_path, dpi=dpi)
    plt.close(fig)
    return True


def max_same_x(x, y, rel_tol=1e-9):
    """
    Take the maximum y over points sharing one offered load.

    Groups points whose x differ by at most rel_tol relatively (exact
    duplicates from retried runs, plus distinct (payload, frequency)
    combos colliding at one product such as 40K@1000Hz vs 4M@10Hz) and
    returns sorted unique-x arrays with max y. The maximum keeps one
    curve per run-kind readable and lines the curve peak up with the
    report table's maximum-achieved value for that run-kind; the
    selection is over one offered load, never across run-kinds.
    """
    x = np.asarray(x, dtype=np.float64)
    y = np.asarray(y, dtype=np.float64)
    order = np.argsort(x, kind='stable')
    x, y = x[order], y[order]
    groups_x, groups_y, cur_x, cur_y = [], [], [], []
    for xi, yi in zip(x.tolist(), y.tolist()):
        if cur_x and abs(xi - cur_x[-1]) <= rel_tol * max(1.0, abs(xi)):
            cur_x.append(xi)
            cur_y.append(yi)
        else:
            if cur_x:
                groups_x.append(sum(cur_x) / len(cur_x))
                groups_y.append(max(cur_y))
            cur_x, cur_y = [xi], [yi]
    if cur_x:
        groups_x.append(sum(cur_x) / len(cur_x))
        groups_y.append(max(cur_y))
    return np.array(groups_x), np.array(groups_y)


def _offered_series(cell, ycol, xmin_bps):
    """
    Build (x, y, case, label) throughput series for one direction cell.

    One connected line per case over offered throughput
    (expected_throughput_Bps), taking the maximum where x collides so
    curve peaks match the report table. Labels use the short case form
    (direction lives in the cell title).
    """
    series = []
    for case in sorted(set(cell['case'])):
        line = cell.loc[cell['case'] == case].sort_values(
            'expected_throughput_Bps')
        if line.empty:
            continue
        x = line['expected_throughput_Bps'].to_numpy(dtype=np.float64)
        y = line[ycol].to_numpy(dtype=np.float64)
        mask = np.isfinite(x) & np.isfinite(y) & (x > 0) & (y > 0)
        x, y = max_same_x(x[mask], y[mask])
        keep = x >= float(xmin_bps)
        x, y = x[keep], y[keep]
        if len(x) == 0:
            continue
        series.append((x, y, case, case_short_label(case)))
    return series


def _parse_xmin(value):
    """Parse --throughput-xmin, falling back to 100MB/s on bad input."""
    try:
        return parse_throughput_label(value or '100MB/s')
    except ValueError:
        return 100e6


def parse_throughput_label(label):
    """Inverse of format_throughput_bps ('40KB/s' -> 40000.0)."""
    text = label.strip()
    if text.endswith('/s'):
        text = text[:-2]
    if text.endswith('B'):
        text = text[:-1]
    mult = 1.0
    if text and text[-1] in 'KMGT':
        mult = {'K': 1e3, 'M': 1e6, 'G': 1e9, 'T': 1e12}[text[-1]]
        text = text[:-1]
    return float(text) * mult


def plot_throughput_vs_expected(per_run, transport, reliability,
                                out_path, dpi=150,
                                xmin_bps=100e6):
    """
    Plot achieved vs expected throughput over all payloads and frequencies.

    Filters per_run to the (transport, reliability) pair, then draws a
    2x3 grid of direction cells (intra-process directions get their own
    cells), one connected line per case over offered throughput
    (target frequency x payload): x is expected, y is achieved average
    throughput, both in bytes/sec with SI units. Points sharing one
    offered load take the maximum; every case keeps its own legend
    entry with a distinct color and symbol. Each cell carries the faint
    ideal diagonal plus the machine memory ceilings (named in the bottom
    caption). The view is cropped at
    xmin_bps (default 100MB/s) so the saturation regime fills the axes
    instead of the overlapping diagonal. Returns True when at least one
    line was drawn.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    work = per_run.copy()
    if 'achieved_throughput_Bps' not in work.columns:
        work = add_throughput_columns(work)
    subset = work.loc[
        (work['transport'] == transport) &
        (work['reliability'] == reliability)]
    if subset.empty:
        sys.stderr.write('throughput %s %s: no data, skipped\n' % (
            transport, reliability))
        return False
    cases = sorted(set(subset['case']))
    if not cases:
        return False
    styles = _series_styles(cases)
    fig, axes = plt.subplots(2, 3, figsize=(18, 10),
                             sharex=True, sharey=True)
    span = _span_subs(
        subset['expected_throughput_Bps'].to_numpy(dtype=np.float64),
        subset['achieved_throughput_Bps'].to_numpy(dtype=np.float64))
    drawn_any = False
    for ax, (group_label, directions) in zip(
            axes.flat, DIRECTION_CELLS):
        cell = subset.loc[subset['direction'].isin(set(directions))]
        ax.set_xscale('log')
        ax.set_yscale('log')
        series = _offered_series(cell, 'achieved_throughput_Bps',
                                 xmin_bps)
        if not series:
            ax.text(0.5, 0.5, 'no data', ha='center', va='center',
                    transform=ax.transAxes)
        else:
            drawn_any = True
            all_x, all_y = [], []
            legend_handles, legend_labels = [], []
            for x, y, key, label in series:
                color, marker = styles[key]
                (line,) = ax.plot(x, y, color=color, marker=marker,
                                  linewidth=2)
                legend_handles.append(line)
                legend_labels.append(label)
                all_x.extend(x.tolist())
                all_y.extend(y.tolist())
            lo = min(min(all_x), min(all_y))
            hi = max(max(all_x), max(all_y))
            lo = max(lo, 1.0)
            ax.plot([lo, hi], [lo, hi], color='#bbbbbb',
                    linestyle='--', linewidth=1.0)
            for value, _name, shade, style in MEMORY_CEILINGS:
                ax.plot([lo, hi], [value, value], color=shade,
                        linestyle=style, linewidth=1.2)
            if legend_handles:
                ax.legend(legend_handles, legend_labels, loc='upper left',
                          fontsize=12)
        ax.set_title(group_label)
        apply_log_ticks(ax, x_kind='throughput_bps',
                        y_kind='throughput_bps', subs=span)
        ax.grid(True, which='both', alpha=0.3)
    if not drawn_any:
        sys.stderr.write('throughput %s %s: nothing above %s, skipped\n'
                         % (transport, reliability,
                            format_throughput_bps(float(xmin_bps))))
        plt.close(fig)
        return False
    fig.suptitle('%s throughput above %s (achieved vs expected, per run)'
                 % (pair_label(transport, reliability),
                    format_throughput_bps(float(xmin_bps))))
    fig.text(0.5, 0.02, 'Expected throughput (payload x frequency)',
             ha='center', fontsize=12)
    fig.text(0.02, 0.5, 'Achieved average throughput', va='center',
             rotation='vertical', fontsize=12)
    ceiling_subtitle(fig, y=0.90)
    fig.tight_layout(rect=[0.04, 0.04, 1, 0.88])
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


def plot_latency_payload_by_transport(per_run, reliability, fixed_case,
                                      fixed_direction, fixed_freq, out_path,
                                      dpi=150):
    """
    Compare transports on one latency-vs-payload sweep.

    Fixes (case, direction, reliability, frequency); x is payload, y is
    per-run p50 latency, one connected line per transport, degraded runs
    included. This is the transpose of plot_sweep_payload: instead of one figure per transport
    with lines per (case, direction), there is one figure with lines per
    transport for direct visual comparison. SI units; fully spelled out
    transport labels.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    subset = per_run.loc[
        (per_run['case'] == fixed_case) &
        (per_run['direction'] == fixed_direction) &
        (per_run['reliability'] == reliability) &
        (np.isclose(per_run['target_frequency_hz'].to_numpy(
            dtype=np.float64), float(fixed_freq)))]
    if subset.empty:
        sys.stderr.write('transport latency comparison %s: no data, '
                         'skipped\n' % reliability)
        return False
    transports = sorted(subset['transport'].unique())
    transports = ([t for t in TRANSPORT_ORDER if t in set(transports)] +
                  sorted(set(transports) - set(TRANSPORT_ORDER)))
    fig, ax = plt.subplots(figsize=(12, 7))
    for transport in transports:
        color = TRANSPORT_COLORS.get(transport, 'k')
        marker = TRANSPORT_MARKERS.get(transport, 'o')
        line = subset.loc[subset['transport'] == transport].sort_values(
            'payload_bytes')
        if line.empty:
            continue
        x = line['payload_bytes'].to_numpy(dtype=np.float64)
        y = line['p50'].to_numpy(dtype=np.float64)
        ax.plot(x, y, label=humanize_transport(transport), color=color,
                marker=marker, linewidth=2)
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Payload')
    ax.set_ylabel('p50 latency')
    ax.set_title('Latency vs payload by transport: %s, %s, %s at %s '
                 '(per run)' % (
                     humanize_case(fixed_case),
                     humanize_direction(fixed_direction),
                     humanize_reliability(reliability),
                     format_freq_pretty(float(fixed_freq))))
    apply_log_ticks(ax, x_kind='payload_bytes', y_kind='latency_us',
                    x_ticks=sorted(subset['payload_bytes'].unique()))
    ax.grid(True, which='both', alpha=0.3)
    handles, _labels = ax.get_legend_handles_labels()
    if handles:
        ax.legend(loc='upper left', fontsize=12)
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


def plot_throughput_frequency_by_transport(per_run, reliability, fixed_case,
                                           fixed_direction, out_path,
                                           dpi=150, xmin_bps=100e6):
    """
    Compare transports on throughput over the full offered-load grid.

    Fixes (case, direction, reliability); x is the expected throughput
    (target frequency x payload, all payloads and frequencies), y is the
    achieved average throughput, one connected line per transport plus a
    dashed ideal y=x line. Shared-x points take the maximum; every transport
    keeps its own legend entry with a distinct color. Bytes/sec with SI units.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    work = per_run.copy()
    if 'achieved_throughput_Bps' not in work.columns:
        work = add_throughput_columns(work)
    subset = work.loc[
        (work['case'] == fixed_case) &
        (work['direction'] == fixed_direction) &
        (work['reliability'] == reliability)]
    if subset.empty:
        sys.stderr.write('transport throughput comparison %s: no data, '
                         'skipped\n' % reliability)
        return False
    transports = sorted(subset['transport'].unique())
    transports = ([t for t in TRANSPORT_ORDER if t in set(transports)] +
                  sorted(set(transports) - set(TRANSPORT_ORDER)))
    series = []
    for transport in transports:
        line = subset.loc[subset['transport'] == transport].sort_values(
            'expected_throughput_Bps')
        if line.empty:
            continue
        x = line['expected_throughput_Bps'].to_numpy(dtype=np.float64)
        y = line['achieved_throughput_Bps'].to_numpy(dtype=np.float64)
        mask = np.isfinite(x) & np.isfinite(y) & (x > 0) & (y > 0)
        x, y = max_same_x(x[mask], y[mask])
        keep = x >= float(xmin_bps)
        x, y = x[keep], y[keep]
        if len(x) == 0:
            continue
        series.append((x, y, transport, humanize_transport(transport)))
    if not series:
        sys.stderr.write('transport throughput comparison %s: nothing '
                         'above %s, skipped\n' % (
                             reliability,
                             format_throughput_bps(float(xmin_bps))))
        return False
    fig, ax = plt.subplots(figsize=(12, 7))
    all_x, all_y = [], []
    legend_handles, legend_labels = [], []
    for x, y, transport, label in series:
        color = TRANSPORT_COLORS.get(transport, 'k')
        marker = TRANSPORT_MARKERS.get(transport, 'o')
        (line,) = ax.plot(x, y, color=color, marker=marker, linewidth=2)
        legend_handles.append(line)
        legend_labels.append(label)
        all_x.extend(x.tolist())
        all_y.extend(y.tolist())
    lo = min(min(all_x), min(all_y))
    hi = max(max(all_x), max(all_y))
    lo = max(lo, 1.0)
    ax.plot([lo, hi], [lo, hi], color='#bbbbbb',
            linestyle='--', linewidth=1.0)
    for value, _name, shade, style in MEMORY_CEILINGS:
        ax.plot([lo, hi], [value, value], color=shade,
                linestyle=style, linewidth=1.2)
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Expected throughput (payload x frequency)')
    ax.set_ylabel('Achieved average throughput')
    fig.suptitle('Throughput by transport above %s: %s, %s, %s (per run)'
                 % (format_throughput_bps(float(xmin_bps)),
                    humanize_case(fixed_case),
                    humanize_direction(fixed_direction),
                    humanize_reliability(reliability)))
    apply_log_ticks(ax, x_kind='throughput_bps', y_kind='throughput_bps',
                    subs=_span_subs(all_x, all_y))
    if float(xmin_bps) > 0:
        ax.set_xlim(left=float(xmin_bps))
    ax.grid(True, which='both', alpha=0.3)
    if legend_handles:
        ax.legend(legend_handles, legend_labels, loc='upper left',
                  fontsize=12)
    ceiling_subtitle(fig, y=0.90)
    fig.tight_layout(rect=[0, 0.02, 1, 0.86])
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


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
    fig, ax = plt.subplots(figsize=(11, 7))
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
                       label=pair_label(transport, reliability),
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
    ax.set_xlabel('Nominal publish period')
    ax.set_ylabel('Inter-send interval std dev')
    ax.set_title('Publish pacing jitter: observed vs configured')
    apply_log_ticks(ax, x_kind='milliseconds', y_kind='milliseconds')
    handles, _labels = ax.get_legend_handles_labels()
    if handles:
        ax.legend(loc='upper left', fontsize=12)
    ax.grid(True, which='both', alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)


def markdown_table(frame):
    """
    Render a DataFrame as a GitHub-flavored markdown table.

    Avoids the tabulate dependency; NaN renders as 'n/a'.
    """
    columns = list(frame.columns)
    rows = [[('n/a' if pd.isna(v) else str(v)) for v in frame[column]]
            for column in columns]
    lines = ['| ' + ' | '.join(columns) + ' |',
             '| ' + ' | '.join(['---'] * len(columns)) + ' |']
    for i in range(len(frame)):
        lines.append('| ' + ' | '.join(row[i] for row in rows) + ' |')
    return '\n'.join(lines)


def pooled_latency_table(pooled, exclude_cases=(), exclude_directions=()):
    """
    Build the pooled latency distribution table over the sweep grid.

    One row per (transport, reliability, case, direction): sample count
    plus p50/p95/p99 pooled across every payload and frequency run of
    that kind, read off the shared log-histograms. Pooling widens each
    distribution to the full grid, which is what makes flatness
    comparable: a tight p50/p95/p99 triple stays tight at any load,
    while payload-sensitive paths fan out. Every compared group ran the
    identical payload x frequency grid, so differences reflect the
    transport and data path, not the mix. No averaging anywhere.
    """
    excluded_cases = set(exclude_cases)
    excluded_dirs = set(exclude_directions)
    rows = []
    for (transport, reliability, case, direction) in sorted(
            pooled['groups']):
        if case in excluded_cases or direction in excluded_dirs:
            continue
        entry = pooled['groups'][
            (transport, reliability, case, direction)]
        rows.append({
            'Transport': humanize_transport(transport),
            'Reliability': humanize_reliability(reliability),
            'Case': humanize_case(case),
            'Direction': humanize_direction(direction),
            'n': '%d' % entry['n'],
            'p50': format_us(pooled_quantile(
                POOLED_LAT_EDGES, entry['hist'], 0.50)),
            'p95': format_us(pooled_quantile(
                POOLED_LAT_EDGES, entry['hist'], 0.95)),
            'p99': format_us(pooled_quantile(
                POOLED_LAT_EDGES, entry['hist'], 0.99)),
        })
    if not rows:
        return 'No runs.'
    return markdown_table(pd.DataFrame(
        rows, columns=['Transport', 'Reliability', 'Case', 'Direction',
                       'n', 'p50', 'p95', 'p99']))


def throughput_max_frame(per_run_fig):
    """
    Compute maximum achieved throughput per run-kind over all payloads.

    Returns a DataFrame indexed by (transport, reliability, case,
    direction) with max_Bps plus the payload and frequency where each
    maximum was hit. A maximum is a selection, not an average: each row
    traces to one run's measured ceiling. Shared by the report table
    and the bar plots so both show the same numbers.
    """
    work = per_run_fig.copy()
    if 'achieved_throughput_Bps' not in work.columns:
        work = add_throughput_columns(work)
    valid = work.dropna(subset=['achieved_throughput_Bps'])
    if valid.empty:
        return pd.DataFrame(
            columns=['transport', 'reliability', 'case', 'direction',
                     'max_Bps', 'at_payload', 'at_freq'])
    best = valid.loc[valid.groupby(
        ['transport', 'reliability', 'case', 'direction'],
        observed=True)['achieved_throughput_Bps'].idxmax()]
    return pd.DataFrame({
        'transport': best['transport'].to_numpy(),
        'reliability': best['reliability'].to_numpy(),
        'case': best['case'].to_numpy(),
        'direction': best['direction'].to_numpy(),
        'max_Bps': best['achieved_throughput_Bps'].to_numpy(
            dtype=np.float64),
        'at_payload': best['payload_bytes'].to_numpy(dtype=np.int64),
        'at_freq': best['target_frequency_hz'].to_numpy(dtype=np.float64),
    })


def throughput_max_table(max_frame):
    """
    Format a throughput-maximum frame (see throughput_max_frame) as
    markdown with SI units and fully spelled out labels.
    """
    if max_frame.empty:
        return 'No runs.'
    frame = pd.DataFrame({
        'Transport': max_frame['transport'].map(humanize_transport),
        'Reliability': max_frame['reliability'].map(humanize_reliability),
        'Case': max_frame['case'].map(humanize_case),
        'Direction': max_frame['direction'].map(humanize_direction),
        'Max achieved': max_frame['max_Bps'].map(
            lambda v: format_throughput_bps(float(v))),
        'At payload': max_frame['at_payload'].map(
            lambda v: format_payload(int(v))),
        'At frequency': max_frame['at_freq'].map(
            lambda v: format_freq_pretty(float(v))),
    })
    return markdown_table(frame)


def plot_corner_bars(pooled, transport, reliability, payload_bytes,
                      frequency_hz, exclude_cases=(), exclude_directions=(),
                      out_path=None, dpi=150):
    """
    Write one 2x3 figure with stacked latency bars per case for one grid
    corner.

    Each cell shows one direction (intra-process directions get their
    own cells); each run-kind gets one stacked bar per cell: the bar
    rises from the bottom to q75, then continues to p95 in full
    saturation, so the IQR bulk and the tail read as two colors on a
    linear scale. The x axis carries positions only; the legend maps
    hues to cases. Corner cells without samples are skipped. Returns
    True when at least one bar was drawn.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    excluded_cases = set(exclude_cases)
    excluded_dirs = set(exclude_directions)
    want_payload = int(payload_bytes)
    want_freq = float(frequency_hz)
    cells = {}
    for key6 in pooled['cells']:
        t, r, case, direction, p, f = key6
        if str(t) != transport or str(r) != reliability:
            continue
        if case in excluded_cases or direction in excluded_dirs:
            continue
        if int(p) != want_payload or not np.isclose(
                float(f), want_freq, rtol=1e-9):
            continue
        entry = pooled['cells'][key6]
        values = np.asarray(entry['res'], dtype=np.float64)
        values = values[np.isfinite(values) & (values > 0)]
        if len(values) == 0:
            continue
        cells.setdefault(str(direction), []).append(
            (str(case), values))
    if not any(cells.values()):
        return False
    styles = _series_styles(
        set(case for rows in cells.values() for case, _v in rows))
    fig, axes = plt.subplots(2, 3, figsize=(18, 10),
                             sharex=False, sharey=True)
    for ax, (group_label, directions) in zip(
            axes.flat, DIRECTION_CELLS):
        rows = []
        for direction in directions:
            rows.extend(cells.get(direction, []))
        rows = sorted(rows, key=lambda row: row[0])
        if not rows:
            ax.text(0.5, 0.5, 'no data', ha='center', va='center',
                    transform=ax.transAxes)
            ax.set_xticks([])
        else:
            from matplotlib.ticker import FuncFormatter
            pos = np.arange(len(rows))
            hues = [styles[case][0] for case, _values in rows]
            q75, p95 = [], []
            for _case, values in rows:
                quartiles = np.percentile(values, [75, 95])
                q75.append(quartiles[0])
                p95.append(quartiles[1])
            q75 = np.asarray(q75)
            p95 = np.asarray(p95)
            ax.bar(pos, q75, width=0.7, bottom=0.0,
                   color=hues, alpha=0.45, edgecolor='black',
                   linewidth=0.5)
            top = ax.bar(pos, p95 - q75, width=0.7, bottom=q75,
                         color=hues, alpha=1.0, edgecolor='black',
                         linewidth=0.5)
            ax.set_xticks(pos)
            ax.set_xticklabels([])
            ax.legend(list(top.patches),
                      [case_short_label(case) for case, _v in rows],
                      loc='upper left', fontsize=12)
        ax.set_title(group_label)
        ax.yaxis.set_major_formatter(
            FuncFormatter(axis_formatter('latency_us')))
        ax.grid(True, axis='y', which='both', alpha=0.3)
    fig.suptitle('Latency by case at %s, %s: %s (IQR bar with p95 tail)' % (
        format_payload(want_payload), format_freq_pretty(want_freq),
        pair_label(transport, reliability)))
    fig.text(0.02, 0.5, 'Latency dispersion', va='center',
             rotation='vertical',
             fontsize=12)
    fig.tight_layout(rect=[0.04, 0.02, 1, 0.94])
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


def plot_throughput_bars(max_frame, reliability, out_path, dpi=150):
    """
    Grouped bars of maximum achieved throughput per run-kind.

    One group per (case, direction), one bar per transport, log y-axis
    with SI bytes/sec. Values come from throughput_max_frame, so the
    bars show exactly the report table's numbers ("populated after the
    table"). Returns True when at least one bar was drawn.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    sub = max_frame.loc[max_frame['reliability'] == reliability]
    if sub.empty:
        sys.stderr.write('throughput bars %s: no data, skipped\n'
                         % reliability)
        return False
    groups = sorted(set(zip(sub['case'], sub['direction'])))
    transports = sorted(sub['transport'].unique())
    if not groups or not transports:
        return False
    lookup = {(c, d, t): v for c, d, t, v in zip(
        sub['case'], sub['direction'], sub['transport'],
        sub['max_Bps'])}
    width = max(10.0, 0.9 * len(groups) * len(transports))
    fig, ax = plt.subplots(figsize=(width, 7))
    x = np.arange(len(groups))
    bar_width = 0.8 / len(transports)
    for i, transport in enumerate(transports):
        heights = [float(lookup.get((c, d, transport), float('nan')))
                   for c, d in groups]
        color = TRANSPORT_COLORS.get(transport, 'k')
        ax.bar(x + (i - (len(transports) - 1) / 2) * bar_width,
               heights, width=bar_width,
               label=humanize_transport(transport), color=color,
               edgecolor='black', linewidth=0.5)
    ax.set_xticks(x, [descriptive_label(c, d) for c, d in groups],
                    rotation=30, ha='right')
    ax.set_yscale('log')
    ax.set_ylabel('Max achieved throughput')
    fig.suptitle('Maximum achieved throughput by run-kind (%s, per run)' %
                 humanize_reliability(reliability))
    apply_log_ticks(ax, y_kind='throughput_bps')
    for value, _name, shade, style in MEMORY_CEILINGS:
        ax.axhline(value, color=shade, linestyle=style, linewidth=1.2)
    ax.grid(True, axis='y', which='both', alpha=0.3)
    handles, _labels = ax.get_legend_handles_labels()
    if handles:
        ax.legend(loc='upper left', fontsize=12)
    ceiling_subtitle(fig, y=0.90)
    fig.tight_layout(rect=[0, 0.02, 1, 0.86])
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


def plot_ontime_vs_offered(per_run, transport, reliability,
                           out_path, dpi=150):
    """
    Plot on-time fraction vs offered throughput.

    Filters per_run to the (transport, reliability) pair, then draws a
    2x3 grid of direction cells (intra-process directions get their own
    cells), one connected line per case: x is the expected throughput
    (target frequency x payload, all payloads and frequencies), y is the
    on-time fraction (receives within one publish period over everything
    published; drops count as missed, floored at 0.01). Log scales on
    both axes over a couple orders of magnitude, so a collapsed corner
    reads as effectively zero. Every case keeps its own legend entry
    with a distinct color and symbol. Returns True when
    at least one line was drawn.
    """
    ensure_backend()
    import matplotlib.pyplot as plt
    from matplotlib.ticker import FuncFormatter, NullFormatter
    work = per_run.copy()
    if 'achieved_throughput_Bps' not in work.columns:
        work = add_throughput_columns(work)
    if 'missed_frac' not in work.columns:
        sys.stderr.write('on-time vs offered %s %s: no missed_frac, '
                         'skipped\n' % (transport, reliability))
        return False
    work['ontime_frac'] = (1.0 - work['missed_frac'].to_numpy(
        dtype=np.float64)).clip(1e-2, 1.0)
    subset = work.loc[
        (work['transport'] == transport) &
        (work['reliability'] == reliability)]
    if subset.empty:
        sys.stderr.write('on-time vs offered %s %s: no data, skipped\n' % (
            transport, reliability))
        return False
    cases = sorted(set(subset['case']))
    if not cases:
        return False
    styles = _series_styles(cases)
    fig, axes = plt.subplots(2, 3, figsize=(18, 10),
                             sharex=True, sharey=True)
    span = _span_subs(
        subset['expected_throughput_Bps'].to_numpy(dtype=np.float64))
    drawn_any = False
    for ax, (group_label, directions) in zip(
            axes.flat, DIRECTION_CELLS):
        cell = subset.loc[subset['direction'].isin(set(directions))]
        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.set_ylim(8e-3, 1.15)
        ax.set_yticks([0.01, 0.1, 1.0])
        ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _p: '%g' % v))
        ax.yaxis.set_minor_formatter(NullFormatter())
        series = _offered_series(cell, 'ontime_frac', 0.0)
        if not series:
            ax.text(0.5, 0.5, 'no data', ha='center', va='center',
                    transform=ax.transAxes)
        else:
            drawn_any = True
            legend_handles, legend_labels = [], []
            for x, y, key, label in series:
                color, marker = styles[key]
                (ln,) = ax.plot(x, y, color=color, marker=marker,
                                linewidth=2)
                legend_handles.append(ln)
                legend_labels.append(label)
            if legend_handles:
                ax.legend(legend_handles, legend_labels, loc='lower left',
                          fontsize=12)
        ax.set_title(group_label)
        apply_log_ticks(ax, x_kind='throughput_bps', subs=span)
        ax.grid(True, which='both', alpha=0.3)
    if not drawn_any:
        sys.stderr.write('on-time vs offered %s %s: nothing valid, '
                         'skipped\n' % (transport, reliability))
        plt.close(fig)
        return False
    fig.suptitle('%s on-time fraction vs offered load (per run)' %
                 pair_label(transport, reliability))
    fig.text(0.5, 0.02, 'Offered throughput (payload x frequency)',
             ha='center', fontsize=12)
    fig.text(0.02, 0.5, 'Fraction on-time (within one period)',
             va='center', rotation='vertical', fontsize=12)
    fig.tight_layout(rect=[0.04, 0.04, 1, 0.94])
    fig.savefig(out_path, dpi=dpi)
    plt.close(fig)
    return True


def write_report(output_dir, per_run, drops, pacing_summary, figures,
                 fixed, pooled, max_frame, image_format='png'):
    """
    Write report.md restructured around per-run statistics.

    Sections: dataset overview, latency sweeps, throughput, transport
    comparisons, pooled-distribution tables plus throughput bars, pooled
    boxes, drop tables, pacing. figures maps logical names to file
    basenames present in output_dir. Lines never average: shared-x
    points take the maximum over one offered load, and every run-kind
    keeps its own legend entry with a distinct color and symbol, but
    every value traces to measured runs.
    """
    lines = []
    lines.append('# Benchmark Analysis Report')
    lines.append('')
    lines.append('Per-run streaming analysis: each run (one case x direction '
                 'x transport x reliability x payload x frequency) is the '
                 'meaningful unit. No statistic combines samples across '
                 'runs. Cross-process end-to-end latency assumes '
                 'NTP-synchronized host clocks.')
    lines.append('')
    lines.append('## 1. Dataset overview')
    lines.append('')
    lines.append('- runs: %d' % len(per_run))
    lines.append('- cases: %s' % ', '.join(sorted(per_run['case'].unique())))
    lines.append('- directions: %s' % ', '.join(
        sorted(per_run['direction'].unique())))
    lines.append('- transports: %s' % ', '.join(
        sorted(per_run['transport'].unique())))
    lines.append('- reliabilities: %s' % ', '.join(
        sorted(per_run['reliability'].unique())))
    lines.append('- payloads: %s' % ', '.join(
        format_payload(int(p)) for p in sorted(
            per_run['payload_bytes'].unique())))
    lines.append('- frequencies: %s' % ', '.join(
        format_freq_hz(float(f)) for f in sorted(
            per_run['target_frequency_hz'].unique())))
    lines.append('- full per-run table: per_run_stats.csv '
                 '(%d rows, one per run_id)' % len(per_run))
    lines.append('')
    excluded_cases = fixed.get('figure_exclude_cases', [])
    excluded_dirs = fixed.get('figure_exclude_directions', [])
    if excluded_cases:
        lines.append('- figures exclude cases: %s' % ', '.join(
            humanize_case(c) for c in excluded_cases))
    if excluded_dirs:
        lines.append('- figures exclude directions: %s' % ', '.join(
            humanize_direction(d) for d in excluded_dirs))
    lines.append('- throughput is payload over time (bytes/sec); '
                 'throughput figures span all payloads including 40M, '
                 'cropped at %s' % format_throughput_bps(
                     float(fixed.get('throughput_xmin_bps', 100e6))))
    line_excluded = fixed.get('line_exclude_cases', [])
    if line_excluded:
        lines.append('- connected-line figures hide cases: %s' % ', '.join(
            humanize_case(c) for c in line_excluded))
    lines.append('- per-run tables keep every payload including 40M, and '
                 'sweep lines draw every run including degraded 40M points: '
                 'latency balloons where delivery collapses, which is the '
                 'honest picture. Manual payload overrides: %s' % (
                     ', '.join(format_payload(int(p)) for p in
                               fixed.get('figure_exclude_payloads', []))
                     or 'none'))
    lines.append('')
    lines.append('Fixed sweep coordinates shared by sweeps and regime bars:')
    lines.append('')
    lines.append('- sweep payloads: %s' % ', '.join(
        format_payload(int(p)) for p in
        fixed.get('sweep_corner_payloads', [])))
    lines.append('- sweep frequencies: %s' % ', '.join(
        format_freq_pretty(float(f)) for f in
        fixed.get('sweep_corner_freqs', [])))
    lines.append('- transport-comparison case/direction/frequency/payload: '
                 '%s, %s, %s, %s' % (
                     humanize_case(fixed['ts_case']),
                     humanize_direction(fixed['ts_direction']),
                     format_freq_pretty(float(fixed['sweep_fixed_freq'])),
                     format_payload(int(fixed['sweep_fixed_payload']))))
    lines.append('')
    lines.append('## 2. Latency sweeps by payload and frequency')
    lines.append('')
    lines.append('One connected line per case with one point per run (p50 '
                 'latency) in a 2x3 grid of direction cells (C++ to C++, '
                 'C++ to C++ intra, C++ to Python, Python to C++, Python '
                 'to Python, Python to Python intra). No error bars. '
                 'Degraded runs are drawn as measured: latency '
                 'balloons where delivery collapses. Axes use SI units '
                 'with shared log scales and grids.')
    lines.append('')
    pairs = sorted(set(zip(per_run['transport'], per_run['reliability'])))
    for transport, reliability in pairs:
        key = '%s_%s' % (transport, reliability)
        entries = []
        for freq in fixed.get('sweep_corner_freqs', []):
            freq_tag = format_freq_hz(float(freq))
            entries.append(
                ('sweep_payload_latency_%s_%s' % (freq_tag, key),
                 'Payload sweep latency at %s (%s)' % (
                     format_freq_pretty(float(freq)),
                     pair_label(transport, reliability))))
        for payload in fixed.get('sweep_corner_payloads', []):
            pay_tag = format_payload(int(payload))
            entries.append(
                ('sweep_freq_latency_%s_%s' % (pay_tag, key),
                 'Frequency sweep latency at %s (%s)' % (
                     format_payload(int(payload)),
                     pair_label(transport, reliability))))
        for kind, title in entries:
            if kind in figures:
                lines.append('### %s' % title)
                lines.append('')
                lines.append('![%s](%s)' % (title, figures[kind]))
                lines.append('')
    lines.append('## 3. Throughput (achieved vs expected)')
    lines.append('')
    lines.append('Achieved average throughput (achieved_hz x payload) vs '
                 'expected throughput (target frequency x payload) over all '
                 'payloads and frequencies, 40M included, cropped at %s '
                 'where transport differences emerge. One connected line '
                 'per case in a 2x3 grid of direction cells, sorted by '
                 'offered load, so '
                 'each curve is throughput as a function of payload x '
                 'frequency. Points sharing one offered load take the '
                 'maximum, so each curve peak matches the report table; '
                 'every run-kind keeps its own legend entry with a '
                 'distinct color and symbol. '
                 'Faint diagonal is the ideal; the knee where lines leave '
                 'it is the saturation point. Dashed horizontals are the '
                 'machine memory ceilings, named in the bottom caption '
                 '(see §5).' % format_throughput_bps(
                     float(fixed.get('throughput_xmin_bps', 100e6))))
    lines.append('')
    for transport, reliability in pairs:
        key = '%s_%s' % (transport, reliability)
        kind = 'throughput_vs_expected_%s' % key
        title = 'Throughput (%s)' % pair_label(transport, reliability)
        if kind in figures:
            lines.append('### %s' % title)
            lines.append('')
            lines.append('![%s](%s)' % (title, figures[kind]))
            lines.append('')
    lines.append('## 4. Transport comparisons (lines per transport)')
    lines.append('')
    lines.append('Direct transport showdown at representative fixed values: '
                 'latency vs payload and throughput over frequency, one line '
                 'per transport. Complements §2-§3, where transport selects '
                 'the figure instead of the line.')
    lines.append('')
    for reliability in sorted(set(per_run['reliability'].unique())):
        for kind, title in (
                ('transport_latency_payload_%s' % reliability,
                 'Latency vs payload by transport (%s)' %
                 humanize_reliability(reliability)),
                ('transport_throughput_freq_%s' % reliability,
                 'Throughput over frequency by transport (%s)' %
                 humanize_reliability(reliability))):
            if kind in figures:
                lines.append('### %s' % title)
                lines.append('')
                lines.append('![%s](%s)' % (title, figures[kind]))
                lines.append('')
    lines.append('## 5. Pooled distributions and throughput ceiling')
    lines.append('')
    lines.append('Machine memory ceilings (DDR4 single-channel 64-bit at '
                 '3200MT/s: 25.6GB/s theoretical) are drawn dashed on every '
                 'throughput figure: %s memcpy, %s memset (40M blocks), %s '
                 'STREAM copy. Filling is a memset; sustained copy paths '
                 'top out at the memcpy line.' % (
                     format_throughput_bps(MEMCPY_CEILING_BPS),
                     format_throughput_bps(MEMSET_CEILING_BPS),
                     format_throughput_bps(STREAM_CEILING_BPS)))
    lines.append('')
    lines.append('Latency: one row per (transport, reliability, case, '
                 'direction) with p50/p95/p99 pooled across every payload '
                 'and frequency run of that kind — the widened distribution '
                 'is what makes flatness comparable, since every group ran '
                 'the identical grid. Throughput: one row per run-kind with '
                 'the maximum achieved average throughput over all payloads '
                 'and frequencies, plus where it was hit, with matching '
                 'bars. Maxima are selections, not averages.')
    lines.append('')
    lines.append('### Pooled latency distribution per run-kind')
    lines.append('')
    lines.append(pooled_latency_table(
        pooled, exclude_cases=fixed.get('figure_exclude_cases', ()),
        exclude_directions=fixed.get('figure_exclude_directions', ())))
    lines.append('')
    lines.append('### Maximum achieved throughput per run-kind')
    lines.append('')
    lines.append(throughput_max_table(max_frame))
    lines.append('')
    for reliability in sorted(set(per_run['reliability'].unique())):
        kind = 'throughput_bars_%s' % reliability
        if kind in figures:
            title = 'Throughput ceiling (%s)' % humanize_reliability(
                reliability)
            lines.append('### %s' % title)
            lines.append('')
            lines.append('![%s](%s)' % (title, figures[kind]))
            lines.append('')
    lines.append('## 6. Regime bars: latency at grid corners')
    lines.append('')
    lines.append('One figure per (payload, frequency) corner per '
                 '(transport, reliability): a 2x3 grid of direction cells '
                 '(intra-process directions get their own cells), one '
                 'stacked bar per case at that corner. Each bar rises '
                 'from the bottom to q75, then continues to p95 in full '
                 'saturation, so the IQR bulk and the tail read as two '
                 'colors on a linear scale. '
                 'Corners are fixed at 4KB/4MB crossed with 10/1000Hz; '
                 'the x axis '
                 'carries positions only, hues map to cases in the '
                 'legend. Copy XCDR is hidden to focus XCDR on '
                 'constrained runs.')
    lines.append('')
    pairs = sorted(set(zip(per_run['transport'], per_run['reliability'])))
    corners = sorted(set(
        key[len('corner_bars_'):] for key in figures
        if key.startswith('corner_bars_')))
    for corner in corners:
        title = corner
        for (transport, reliability) in pairs:
            suffix = '_%s_%s' % (transport, reliability)
            if corner.endswith(suffix):
                tag = corner[:-len(suffix)]
                payload_label, _, freq_label = tag.partition('_')
                title = '%s at %s, %s' % (
                    format_payload_si(float(parse_payload_label(
                        payload_label))),
                    format_freq_pretty(parse_freq_label(freq_label)),
                    pair_label(transport, reliability))
                break
        lines.append('### Corner %s' % title)
        lines.append('')
        for name in sorted(figures):
            if name == 'corner_bars_%s' % corner:
                lines.append('![%s](%s)' % (name, figures[name]))
                lines.append('')
    lines.append('## 7. Drop Analysis')
    lines.append('')
    lines.append('On-time fraction per run (receives within one publish '
                 'period over everything published; drops count as missed) '
                 'over offered load, one line per run-kind, log scales. '
                 'Collapsed corners read as effectively zero.')
    lines.append('')
    for transport, reliability in sorted(set(zip(
            per_run['transport'], per_run['reliability']))):
        kind = 'ontime_vs_offered_%s_%s' % (transport, reliability)
        if kind in figures:
            title = 'On-time fraction (%s)' % pair_label(
                transport, reliability)
            lines.append('### %s' % title)
            lines.append('')
            lines.append('![%s](%s)' % (title, figures[kind]))
            lines.append('')
    lines.append('')
    if len(drops):
        lines.append('Runs with drops > 0 (from per-run n_publish / '
                     'n_receive):')
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
    lines.append('## 8. Pacing and Publish Jitter')
    lines.append('')
    if 'pacing' in figures:
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
    parser.add_argument('--exclude-payloads', default='',
                        help='Comma-separated payload labels hidden from '
                             'sweep figures as a manual override (CSVs keep '
                             'all runs; degraded runs are already excluded '
                             'point-wise by the full-delivery rule; '
                             'default: show all)')
    parser.add_argument('--time-series-steps', default=None,
                        help='Deprecated: ignored (kept for CLI compat).')
    parser.add_argument('--sweep-fixed-freq', default=None,
                        help='Fixed frequency label for payload sweeps '
                             '(default: dataset median)')
    parser.add_argument('--sweep-fixed-payload', default=None,
                        help='Fixed payload label for frequency sweeps '
                             '(default: dataset median)')
    parser.add_argument('--ts-case', default=None,
                        help='Representative case for transport-comparison '
                             'figures '
                             '(default: constrained_pub_only case)')
    parser.add_argument('--ts-direction', default=None,
                        help='Representative direction for '
                             'transport-comparison figures '
                             '(default: cpp_to_py)')
    parser.add_argument('--figure-exclude-cases', default='exp_copy_xcdr',
                        help='Comma-separated cases hidden from figures '
                             '(CSVs keep all runs; default: exp_copy_xcdr; '
                             'pass empty string to show all)')
    parser.add_argument('--figure-exclude-directions', default='',
                        help='Comma-separated directions hidden from figures '
                             '(CSVs keep all runs; default: show all)')
    parser.add_argument('--line-exclude-cases', default='std_copy_xcdr',
                        help='Comma-separated cases hidden from connected-'
                             'line figures and distribution boxes '
                             '(tables, pacing and CSVs keep them; '
                             'default: std_copy_xcdr; pass empty string '
                             'to show all)')
    parser.add_argument('--throughput-xmin', default='100MB/s',
                        help='Crop throughput figures below this offered '
                             'load so the saturation regime fills the axes '
                             '(default: 100MB/s)')
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
    """
    Run the streaming per-run pipeline (bounded memory).

    Streams one CSV at a time into per-run stats + pacing, then renders
    figures from the small accumulator tables. No step holds more than
    one file's rows plus the small accumulator tables.
    """
    output_dir = args.output_dir or os.path.join(args.results_dir,
                                                 'analysis')
    os.makedirs(output_dir, exist_ok=True)
    cases = set(parse_csv_list(args.cases)) if args.cases else None
    transports = (set(parse_csv_list(args.transports))
                  if args.transports else None)
    directions = (set(parse_csv_list(args.directions))
                  if args.directions else None)
    if getattr(args, 'time_series_steps', None):
        sys.stderr.write(
            'warning: --time-series-steps is deprecated and ignored\n')

    # Per-run tables keep every payload including 40M: payload exclusion
    # is figure-level only (all figures except throughput views), while
    # throughput deliberately includes 40M so saturation stays visible.
    per_run, pacing, pooled = collect_per_run_tables(
        args.results_dir, skip_dirs=(output_dir,), cases=cases,
        transports=transports, directions=directions)
    per_run = add_throughput_columns(per_run)
    drops = drops_table(per_run)
    pacing_summary = pacing_summary_table(pacing)
    per_run.to_csv(os.path.join(output_dir, 'per_run_stats.csv'),
                   index=False)
    drops.to_csv(os.path.join(output_dir, 'drops.csv'), index=False)
    pacing.to_csv(os.path.join(output_dir, 'pacing.csv'), index=False)
    pacing_summary.to_csv(
        os.path.join(output_dir, 'pacing_summary.csv'), index=False)

    fixed = resolve_fixed_values(per_run, args)
    per_run_fig = filter_figure_runs(
        per_run, exclude_cases=fixed.get('figure_exclude_cases', ()),
        exclude_directions=fixed.get('figure_exclude_directions', ()),
        exclude_payloads=fixed.get('figure_exclude_payloads', ()))
    # Throughput scope: same case/direction exclusions, but every
    # payload including 40M, so saturation regimes stay visible.
    per_run_sat = filter_figure_runs(
        per_run, exclude_cases=fixed.get('figure_exclude_cases', ()),
        exclude_directions=fixed.get('figure_exclude_directions', ()))
    # Connected-line scope: Copy XCDR tracks Copy FastCDR within
    # distribution and is hidden from line plots (kept everywhere else).
    line_excluded = set(fixed.get('line_exclude_cases', ()))
    per_run_lines = per_run_fig.loc[
        ~per_run_fig['case'].isin(line_excluded)].copy()
    per_run_sat_lines = per_run_sat.loc[
        ~per_run_sat['case'].isin(line_excluded)].copy()
    if per_run_fig.empty:
        sys.stderr.write(
            'warning: figure exclusions removed all runs; '
            'figures fall back to unfiltered runs\n')
        per_run_fig = per_run.copy()
        per_run_lines = per_run.copy()
    # Maxima run over the 40M-inclusive set (same scope as the
    # throughput figures), never the payload-trimmed figure set.
    max_frame = throughput_max_frame(per_run_sat)
    xmin_bps = float(fixed.get('throughput_xmin_bps', 100e6))

    figures = {}
    fmt = args.format
    if not args.no_figures:
        # No pacing figure: publish-timing scatters proved uninformative;
        # pacing CSVs and the summary table below still ship.
        pairs_lines = sorted(set(zip(per_run_lines['transport'],
                                     per_run_lines['reliability'])))
        pairs_sat = sorted(set(zip(per_run_sat_lines['transport'],
                                    per_run_sat_lines['reliability'])))
        # Fixed sweep coordinates shared by sweeps and regime bars:
        # corner payloads/frequencies snapped to the figure-visible grid.
        fig_payloads = sorted(set(int(p) for p in
                                  per_run_fig['payload_bytes'].unique()))
        fig_freqs = sorted(set(float(f) for f in
                               per_run_fig['target_frequency_hz'].unique()))
        sweep_payloads, sweep_freqs = [], []
        if fig_payloads and fig_freqs:
            for payload in (_snap_to_grid(CORNER_SMALL_PAYLOAD,
                                          fig_payloads),
                            _snap_to_grid(CORNER_LARGE_PAYLOAD,
                                          fig_payloads)):
                if payload not in sweep_payloads:
                    sweep_payloads.append(payload)
            for freq in (_snap_to_grid(CORNER_LOW_FREQ, fig_freqs),
                         _snap_to_grid(CORNER_HIGH_FREQ, fig_freqs)):
                if freq not in sweep_freqs:
                    sweep_freqs.append(freq)
        fixed['sweep_corner_payloads'] = sweep_payloads
        fixed['sweep_corner_freqs'] = sweep_freqs
        corners = [(payload, freq) for payload in sweep_payloads
                   for freq in sweep_freqs]
        for transport, reliability in pairs_lines:
            key = '%s_%s' % (transport, reliability)
            for freq in sweep_freqs:
                freq_tag = format_freq_hz(float(freq))
                lat = 'fig_sweep_payload_latency_%s_%s.%s' % (
                    freq_tag, key, fmt)
                if plot_sweep_payload(
                        per_run_lines, transport, reliability, freq,
                        os.path.join(output_dir, lat), dpi=args.dpi):
                    figures['sweep_payload_latency_%s_%s' % (
                        freq_tag, key)] = lat
            for payload in sweep_payloads:
                pay_tag = format_payload(int(payload))
                lat = 'fig_sweep_freq_latency_%s_%s.%s' % (
                    pay_tag, key, fmt)
                if plot_sweep_frequency(
                        per_run_lines, transport, reliability, payload,
                        os.path.join(output_dir, lat), dpi=args.dpi):
                    figures['sweep_freq_latency_%s_%s' % (
                        pay_tag, key)] = lat
        for transport, reliability in pairs_sat:
            key = '%s_%s' % (transport, reliability)
            # Throughput uses the 40M-inclusive set so the headline
            # regime (e.g. ~11GB/s for C++ to C++) stays visible.
            thr = 'fig_throughput_vs_expected_%s.%s' % (key, fmt)
            if plot_throughput_vs_expected(
                    per_run_sat_lines, transport, reliability,
                    os.path.join(output_dir, thr), dpi=args.dpi,
                    xmin_bps=xmin_bps):
                figures['throughput_vs_expected_%s' % key] = thr
        # Representative case/direction for transport-comparison figures,
        # falling back when absent from figure data.
        rep_case = fixed['ts_case']
        if rep_case not in set(per_run_fig['case'].unique()):
            rep_case = resolve_default_case(per_run_fig, None)
        rep_direction = fixed['ts_direction']
        if rep_direction not in set(
                per_run_fig['direction'].astype(str).unique()):
            vals = sorted(per_run_fig['direction'].astype(str).unique())
            rep_direction = vals[0] if vals else rep_direction
        # Direct transport comparisons at representative fixed values: one
        # latency-vs-payload and one throughput-over-frequency figure per
        # reliability, lines per transport.
        for reliability in sorted(per_run_fig['reliability'].unique()):
            lat_name = 'fig_transport_latency_payload_%s.%s' % (
                reliability, fmt)
            if plot_latency_payload_by_transport(
                    per_run_lines, reliability, rep_case,
                    rep_direction, fixed['sweep_fixed_freq'],
                    os.path.join(output_dir, lat_name), dpi=args.dpi):
                figures['transport_latency_payload_%s'
                        % reliability] = lat_name
            thr_name = 'fig_transport_throughput_freq_%s.%s' % (
                reliability, fmt)
            if plot_throughput_frequency_by_transport(
                    per_run_sat_lines, reliability, rep_case,
                    rep_direction,
                    os.path.join(output_dir, thr_name), dpi=args.dpi,
                    xmin_bps=xmin_bps):
                figures['transport_throughput_freq_%s'
                        % reliability] = thr_name
        # Regime bars: one figure per (payload, frequency) corner per
        # (transport, reliability), stacked quartile bars per run-kind
        # at that corner. Corners are fixed at 4KB/4MB crossed with
        # 10/1000Hz, snapped to the figure-visible grid.
        box_pairs = sorted(set(zip(per_run_fig['transport'],
                                      per_run_fig['reliability'])))
        box_exclude_cases = (set(fixed.get('figure_exclude_cases', ())) |
                             set(fixed.get('line_exclude_cases', ())))
        for corner_payload, corner_freq in corners:
            corner_tag = '%s_%s' % (format_payload(corner_payload),
                                    format_freq_hz(corner_freq))
            for transport, reliability in box_pairs:
                box_name = 'fig_corner_bars_%s_%s_%s.%s' % (
                    corner_tag, transport, reliability, fmt)
                if plot_corner_bars(
                        pooled, transport, reliability,
                        corner_payload, corner_freq,
                        exclude_cases=box_exclude_cases,
                        exclude_directions=fixed.get(
                            'figure_exclude_directions', ()),
                        out_path=os.path.join(output_dir, box_name),
                        dpi=args.dpi):
                    figures['corner_bars_%s_%s_%s' % (
                        corner_tag, transport, reliability)] = box_name
        # Missed-or-late vs offered load: one figure per (transport,
        # reliability) over the full grid, same scope as throughput.
        for transport, reliability in pairs_sat:
            key = '%s_%s' % (transport, reliability)
            ontime = 'fig_ontime_vs_offered_%s.%s' % (key, fmt)
            if plot_ontime_vs_offered(
                    per_run_sat_lines, transport, reliability,
                    os.path.join(output_dir, ontime), dpi=args.dpi):
                figures['ontime_vs_offered_%s' % key] = ontime
        for reliability in sorted(max_frame['reliability'].unique()) \
                if not max_frame.empty else []:
            bar_name = 'fig_throughput_bars_%s.%s' % (reliability, fmt)
            if plot_throughput_bars(
                    max_frame, reliability,
                    os.path.join(output_dir, bar_name), dpi=args.dpi):
                figures['throughput_bars_%s' % reliability] = bar_name
    if not args.no_report:
        write_report(output_dir, per_run, drops, pacing_summary, figures,
                     fixed, pooled, max_frame, image_format=fmt)
    return 0


def main(argv=None):
    """Parse arguments and run the pipeline."""
    parser = build_parser()
    return run_pipeline(parser.parse_args(argv))


if __name__ == '__main__':
    sys.exit(main())
