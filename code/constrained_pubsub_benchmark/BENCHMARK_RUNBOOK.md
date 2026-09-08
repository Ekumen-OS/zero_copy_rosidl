# Constrained Pub/Sub Benchmark Runbook

How to run the REP-0157 constrained pub/sub benchmark suite: three
orthogonal case axes (message x data path x backend), six process
directions, three DDS transports, stepped payload and frequency grids.
Every run emits raw per-sample CSV; post-processing derives latency,
throughput, and plots.

## 1. Build

```bash
cd ~/ws
source /opt/ros/jazzy/setup.bash
MAKEFLAGS=-j2 colcon build --executor sequential \
  --packages-up-to constrained_pubsub_benchmark
```

## 2. Environment

```bash
cd ~/ws
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
BIN=install/constrained_pubsub_benchmark/lib/constrained_pubsub_benchmark
```

## 3. Benchmark matrix

Case axes (`--message` x `--config` x `--backend`):

| Axis | Values |
|------|--------|
| `--message` | `std` (`sensor_msgs/msg/Image`), `exp` (`sensor_msgs/msg/experimental/Image`) |
| `--config` | `copy` (plain copy), `constrained_pub_only` (constrained loaned publisher), `constrained_pub_sub` (constrained loaned pub + sub) |
| `--backend` | `auto` (default: FastCDR for a `std` copy, XCDR elsewhere), `fastcdr` (copy only), `xcdr` |

Constrained configs require `--message exp`; `--backend fastcdr`
requires `--config copy`. The five benchmarked cases are
`std+copy+fastcdr`, `std+copy+xcdr`, `exp+copy+xcdr`,
`exp+constrained_pub_only+xcdr`, and `exp+constrained_pub_sub+xcdr`.

Directions (`--direction`): `cpp_to_cpp`, `cpp_to_py`, `py_to_cpp`,
`py_to_py` (two processes), `intra_cpp`, `intra_py` (one process; ROS
intra-process comms are disabled so DDS transports are exercised).

Transports (`--transport`, Fast DDS XML profiles under `profiles/`):

| ID | Transports | Data sharing |
|----|-----------|--------------|
| `udp` | UDPv4 only | OFF |
| `shmem` | SHM only (64 MB segment, 48 MB max) | OFF |
| `shmem_ds` | SHM only | AUTOMATIC for the benchmark topic (rendered per step, per role) |

`shmem_ds` needs `RMW_FASTRTPS_USE_QOS_FROM_XML=1` (set automatically by
the orchestrator) and ~64 MB of free `/dev/shm` per participant. Data
sharing stays AUTOMATIC, never ON: a writer with ON and DYNAMIC_REUSABLE
is a configuration failure, while AUTOMATIC lets such a writer gracefully
skip data sharing. The orchestrator renders two files per step —
`ds_profile_pub.xml` (data-sharing capable writer) and
`ds_profile_sub.xml` (dynamic receiving end; also used by intra-process
runners) — and only releases the publisher (stdin go-signal) after both
ends report mutual discovery (`PUB_READY` + `SUB_MATCHED`).

Default sweep grids: payloads `4,40,400,4K,40K,400K,4M,40M`; frequencies
`10^(k/3)` Hz for k=0..9 (1, 2.15, 4.64, 10, 21.5, 46.4, 100, 215, 464,
1000 Hz); transports `udp,shmem,shmem_ds`; 10 s per step.

## 4. Single runs

One publisher/subscriber pair (or one intra-process runner), merged raw
CSV on stdout, diagnostics on stderr:

```bash
# C++ constrained pub/sub over shared memory, 4 KB at 100 Hz for 10 s
python3 $BIN/inter_proc_benchmark.py --direction cpp_to_cpp \
  --message exp --config constrained_pub_sub --transport shmem \
  --payload-bytes 4K --publish-rate-hz 100 --duration-sec 10 \
  --grace-sec 2 > /tmp/run.csv

# Same-process Python baseline, standard message, FastCDR
python3 $BIN/inter_proc_benchmark.py --direction intra_py \
  --message std --config copy --transport udp \
  --payload-bytes 400 --publish-rate-hz 10 --duration-sec 10 \
  > /tmp/intra.csv

# Manual halves (unique topic per run; subscriber first, then publisher).
# The publisher prints PUB_READY after matching, then blocks reading a
# go-signal from stdin; it publishes only after the subscriber prints
# SUB_MATCHED (receive window open), so no head samples are lost.
TOPIC=/manual_$RANDOM
$BIN/inter_proc_cpp_subscriber --message exp --config constrained_pub_sub \
  --transport shmem --payload-bytes 4K --publish-rate-hz 10 \
  --duration-sec 10 --run-id manual1 --direction cpp_to_cpp \
  --topic $TOPIC > /tmp/sub.csv 2> /tmp/sub.err &
sleep 1
echo go | $BIN/inter_proc_cpp_publisher --message exp \
  --config constrained_pub_sub --transport shmem \
  --payload-bytes 4K --publish-rate-hz 10 --duration-sec 10 \
  --run-id manual1 --direction cpp_to_cpp --topic $TOPIC > /tmp/pub.csv 2> /tmp/pub.err
```

## 5. Stepped sweeps

```bash
# Full default grid for one case x direction (8 x 10 x 3 = 240 steps)
python3 $BIN/inter_proc_benchmark.py --sweep --direction cpp_to_cpp \
  --message exp --config constrained_pub_sub > /tmp/sweep.csv 2> /tmp/sweep.log

# Reduced grid for a quick comparison, with per-step logs
python3 $BIN/inter_proc_benchmark.py --sweep --direction py_to_py \
  --message exp --config constrained_pub_sub \
  --sweep-payloads 4K,400K,4M --sweep-freqs 10,100,1000 \
  --sweep-shm udp,shmem --duration-per-step 10 \
  --prof-dir /tmp/sweep_prof > /tmp/sweep_py.csv 2> /tmp/sweep_py.log
```

A full matrix (5 cases x 6 directions) is 30 sweep invocations of up to
240 steps each. Steps run sequentially with a unique topic and run id;
a failed step emits an error row and the sweep continues (nonzero exit
if any step failed).

## 6. Collecting the matrix

Two driver scripts (package root, also installed to
`share/constrained_pubsub_benchmark`) run whole sets sequentially with
`[i/N]` progress. Both resume: an invocation with a `.done` marker in
the results dir is skipped; failures keep a `.csv.failed` copy plus the
stderr log and are retried on the next run. `RESULTS_DIR` overrides the
output location; `DRY_RUN=1` prints every command without running.

Machine hygiene: each step cleans stale FastDDS SHM segments
(`fast_datasharing_*`, `fastrtps_*`), appends a temp/freq/load/SHM
snapshot to `{RESULTS_DIR}/machine_state.log`, and cools down until max
CPU temp drops below `COOLDOWN_MAX_C` (default 75°C, `COOLDOWN_TIMEOUT`
120 s cap) before the next step. Heat soak skews latencies ~2x, so do
not disable this for publishable runs. `ROS_DISABLE_LOANED_MESSAGES=0`
is exported by both drivers (loaned takes are opt-in at the rcl layer),
and both pass `--fill-encoding 64 --fill-frame-id 16 --fill-data-ratio
1.0` so constrained messages fill their bounds and skip the compaction
rewrite.
```bash
# Pilot first: 5 cases x 4 inter-process directions, shmem_ds only,
# 10 Hz, 100K + 1MB payloads, 30 s per step (~25 min)
./src/constrained_pubsub_benchmark/run_pilot.sh

# Full matrix: 5 cases x 6 directions x 240 steps at 30 s (~34 h)
RESULTS_DIR=/data/matrix ./src/constrained_pubsub_benchmark/run_matrix.sh
```

Run from the workspace root, one driver at a time (SHM segments and
40 MB payloads are sized for sequential steps). Inspect a finished
invocation's `.log` on failure, delete a stale `.done` marker to force
a re-run, then feed the results dir to `analyze_results.py` (§8).

## 7. Raw CSV schema

One row per published (`publish`) or received (`receive`) sample, plus
`error` rows for explicit failures (never sequence-joined):

```text
run_id,event,process,direction,config,message,backend,transport,
payload_bytes,target_frequency_hz,step_index,phase,sequence,
observed_utc,observed_mono_ns,send_mono_ns,receive_mono_ns,
middleware_source_ns,middleware_received_ns,status,error
```

Join `publish`/`receive` rows on `(run_id, sequence)`; a published
sequence with no receive row is a drop. Derive per-sample end-to-end
latency as `receive_mono_ns - send_mono_ns` and transport latency as
`middleware_received_ns - middleware_source_ns` (example):

```python
import csv
from collections import defaultdict
pub, sub = {}, defaultdict(list)
for row in csv.DictReader(open('/tmp/sweep.csv')):
    key = (row['run_id'], int(row['sequence']))
    if row['event'] == 'publish':
        pub[key] = int(row['send_mono_ns'])
    elif row['event'] == 'receive':
        sub[key].append(
            (int(row['receive_mono_ns']) - pub[key]) / 1000.0)
lat = [v[0] for v in sub.values() if v]
drops = sum(1 for k in pub if k not in sub)
```

## 8. Post-processing

`scripts/analyze_results.py` turns raw sweep CSVs into statistics,
figures, and a Markdown report (spec: `src/PLAN_postprocessing.md`).
Needs pandas, numpy, matplotlib (pip). The script is a source-tree
tool; it is not installed by the package build.

```bash
# Analyze a results directory (raw CSVs anywhere underneath)
python3 src/constrained_pubsub_benchmark/scripts/analyze_results.py \
  /tmp/sweep_results --output-dir /tmp/sweep_results/analysis

# Common filters
python3 src/constrained_pubsub_benchmark/scripts/analyze_results.py \
  /tmp/sweep_results --cases exp_constrained_pub_sub_xcdr \
  --transports shmem,shmem_ds --no-figures
```

Inputs: raw CSV files (21 columns) anywhere under the results
directory. Outputs under `{results_dir}/analysis` (override with
`--output-dir`): six CSVs (`summary_matrix`, `per_run_stats`,
`sweep_by_payload`, `sweep_by_frequency`, `sweep_by_direction`,
`drops`), 27 PNG figures (summary heatmap, per-case payload and
frequency sweeps, per-step time series), and `report.md`. See
`--help` for step selection, payload exclusion, PDF output, and DPI.

## 9. What to look at

- **stdout CSV**: the only benchmark output. One header line, then data
  rows. Missing receive rows are drops, not errors.
- **stderr**: `READY` handshake, `DONE sent/received` summaries, per-step
  progress (`[i/N] run_id`), malformed-row warnings, failure messages.
- **`--prof-dir`**: per-process (per-step) stdout/stderr logs, including
  FastDDS traces. Relevant markers:
  - `XcdrTypeSupport::serialize type=N` — serialize path
    (2 = ROS_MESSAGE, 0 = CDR_BUFFER).
  - `XcdrTypeSupport::deserialize type=N` — deserialize path
    (0 = ROS_MESSAGE_LOAN cast, 2 = ROS_MESSAGE full deserialize).
  - `check_datasharing_compatible ... is_bounded=...` — data sharing enabled?
  - `XcdrTypeSupport type=... bounded=... type_size=...` — bounded/plain flags.

## 10. Known behaviors (not bugs)

- **Short-window tail drops**: with 1–3 s runs, discovery settle (~1 s)
  plus the grace window can clip trailing messages. They appear as
  published-without-received rows, exactly as designed. Use the default
  10 s steps for publishable drop rates.
- **Unbounded messages + `shmem_ds` fall back gracefully**: unbounded
  types cannot use data sharing; with AUTOMATIC the endpoints skip it and
  run over plain SHM transport. Compare against `shmem` to isolate the
  transport effect; only constrained publishers engage data sharing.
- **40 MB steps** need ~64 MB free `/dev/shm` per participant for the SHM
  modes; UDP mode fragments heavily at that size (that is the point).
