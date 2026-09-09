#!/usr/bin/env bash
#
# Full matrix driver: 5 cases x 6 directions x 2 reliabilities, one sweep
# invocation each (7 payloads x 6 frequencies x 3 transports = 126 steps
# at 30 s). Reliability is a first-class run_id coordinate, so both
# levels share one results directory and post-processing separates them.
#
# Pure publish time: 60 invocations x 126 steps x 30 s ~= 63 h, plus
# per-step discovery/settle overhead (~20%) and thermal cooldown gaps.
# Payloads drop the degenerate 4B point; frequencies keep decade anchors
# (1, 10, 100, 1000) plus mids (46.4, 464) to compensate for the added
# reliability axis.
#
# Resumable: an invocation is skipped when its .done marker exists in the
# results dir. Partial output lands in .csv.tmp while running and is moved
# into place only on success; failures keep a .csv.failed copy and are
# re-run on the next invocation of this script. Progress ([i/N], elapsed,
# average pace) prints throughout; a failure summary prints at the end.
#
# Usage (source tree, from the colcon workspace root):
#   ./src/constrained_pubsub_benchmark/scripts/run_matrix.sh
#   RESULTS_DIR=/data/matrix ./src/constrained_pubsub_benchmark/scripts/run_matrix.sh
#   DRY_RUN=1 ./src/constrained_pubsub_benchmark/scripts/run_matrix.sh  # list only
# Or installed (any directory, environment sourced):
#   ros2 run constrained_pubsub_benchmark matrix
#

RESULTS_DIR="${RESULTS_DIR:-/tmp/matrix_results}"
DRY_RUN="${DRY_RUN:-0}"
STEP_SEC=30

# Resolve install-vs-source layout. Via `ros2 run`, this script lives in
# install/<pkg>/lib/<pkg>/ next to the benchmark executables (environment
# already sourced by the caller). Otherwise it runs from the source tree,
# rooted at the colcon workspace.
HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
if [[ -f "$HERE/inter_proc_cpp_publisher" ]]; then
  BIN="$HERE"
  ANALYZE="$HERE/analyze"
else
  if [[ ! -f install/setup.bash ]]; then
    echo "error: run from the colcon workspace root" >&2
    exit 2
  fi
  # shellcheck disable=SC1091
  source install/setup.bash
  BIN="install/constrained_pubsub_benchmark/lib/constrained_pubsub_benchmark"
  ANALYZE="src/constrained_pubsub_benchmark/scripts/analyze_results.py"
fi
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
# Loaned takes are opt-in at the rcl layer (default off); the constrained
# benchmark needs them for the zero-copy take path (see BENCHMARK_RUNBOOK).
export ROS_DISABLE_LOANED_MESSAGES=0

CASES=(
  "std copy fastcdr"
  "std copy xcdr"
  "exp copy xcdr"
  "exp constrained_pub_only xcdr"
  "exp constrained_pub_sub xcdr"
)
DIRECTIONS=(
  cpp_to_cpp cpp_to_py py_to_cpp py_to_py intra_cpp intra_py
)
RELIABILITIES=(
  reliable best_effort
)
# Reduced sweep grids (see header): 4B dropped, 6 frequencies.
SWEEP_PAYLOADS="40,400,4K,40K,400K,4M,40M"
SWEEP_FREQS="1,10,46.4,100,464,1000"

TOTAL=$(( ${#CASES[@]} * ${#DIRECTIONS[@]} * ${#RELIABILITIES[@]} ))
mkdir -p "$RESULTS_DIR"
FAILURES=()
START_ALL=$SECONDS
i=0

# --- machine hygiene (thermal + SHM) ---
# FastDDS SHM segments leak on unclean exits and heat soak skews latencies
# ~2x, so each step cleans stale segments, logs machine state, and cools
# down before the next step. Tunables (env overrides):
#   COOLDOWN_MAX_C   - wait until max CPU temp drops below this (default 75)
#   COOLDOWN_TIMEOUT - max seconds to wait per step (default 120)
#   COOLDOWN_POLL    - seconds between temp checks (default 5)
#   COOLDOWN_SEC     - fixed sleep when no sensors found (default 10, 0 disables)
COOLDOWN_MAX_C="${COOLDOWN_MAX_C:-75}"
COOLDOWN_TIMEOUT="${COOLDOWN_TIMEOUT:-120}"
COOLDOWN_POLL="${COOLDOWN_POLL:-5}"
COOLDOWN_SEC="${COOLDOWN_SEC:-10}"

cpu_max_temp_c() {
  local max="" t f
  for f in /sys/class/thermal/thermal_zone*/temp; do
    [[ -r "$f" ]] || continue
    t=$(cat "$f" 2>/dev/null) || continue
    [[ "$t" =~ ^[0-9]+$ ]] || continue
    t=$((t / 1000))
    if [[ -z "$max" || "$t" -gt "$max" ]]; then max="$t"; fi
  done
  echo "$max"
}

machine_snapshot() {  # $1 = tag
  local temps="n/a" freqs="n/a" temp
  temp=$(cpu_max_temp_c)
  [[ -n "$temp" ]] && temps="${temp}C"
  if grep -q . /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null; then
    freqs=$(awk '{s+=$1; n++} END {if (n) printf "%dMHz", s/n/1000}' /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq 2>/dev/null)
    [[ -z "$freqs" ]] && freqs="n/a"
  fi
  printf '%s %-12s temp=%s freq=%s load=%s shm=%s\n' \
    "$(date -u +%FT%TZ)" "$1" "$temps" "$freqs" \
    "$(cut -d' ' -f1-3 /proc/loadavg)" \
    "$(df -h /dev/shm 2>/dev/null | awk 'NR==2{print $3"/"$2}')" \
    >> "$RESULTS_DIR/machine_state.log"
}

shm_cleanup() {
  # Only FastDDS SHM prefixes; drivers run one at a time (see runbook §6),
  # so no live benchmark can hold these when this runs pre-step.
  rm -f /dev/shm/fast_datasharing_* /dev/shm/fastrtps_* 2>/dev/null || true
}

cooldown() {
  local temp deadline=$((SECONDS + COOLDOWN_TIMEOUT))
  temp=$(cpu_max_temp_c)
  if [[ -z "$temp" ]]; then
    echo "  cooldown: no temp sensors, sleeping ${COOLDOWN_SEC}s"
    sleep "$COOLDOWN_SEC"
    return 0
  fi
  while [[ "$temp" -gt "$COOLDOWN_MAX_C" && $SECONDS -lt $deadline ]]; do
    echo "  cooldown: temp ${temp}C > ${COOLDOWN_MAX_C}C, waiting..."
    sleep "$COOLDOWN_POLL"
    temp=$(cpu_max_temp_c)
    [[ -z "$temp" ]] && break
  done
  temp=$(cpu_max_temp_c)
  echo "  cooldown: done (temp ${temp:-n/a}C)"
}

run_one() {
  local name="$1"
  shift
  i=$((i + 1))
  local out="$RESULTS_DIR/$name.csv"
  local log="$RESULTS_DIR/$name.log"
  local done="$RESULTS_DIR/$name.done"
  if [[ -f "$done" ]]; then
    echo "[$i/$TOTAL] SKIP $name (already done)"
    return 0
  fi
  local elapsed=$((SECONDS - START_ALL))
  echo "[$i/$TOTAL] START $name (elapsed ${elapsed}s)"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "  python3 $BIN/inter_proc_benchmark.py $*"
    return 0
  fi
  machine_snapshot "start:$name"
  shm_cleanup
  python3 "$BIN/inter_proc_benchmark.py" "$@" > "$out.tmp" 2> "$log"
  local rc=$?
  if [[ $rc -eq 0 ]]; then
    mv "$out.tmp" "$out"
    touch "$done"
    echo "[$i/$TOTAL] DONE $name"
  else
    mv "$out.tmp" "$out.failed"
    echo "[$i/$TOTAL] FAIL $name (rc=$rc, see $log)"
    FAILURES+=("$name")
  fi
  machine_snapshot "end:$name"
  cooldown
}

for rel in "${RELIABILITIES[@]}"; do
  for entry in "${CASES[@]}"; do
  # shellcheck disable=SC2086
  set -- $entry
  msg="$1"
  cfg="$2"
  be="$3"
  for dir in "${DIRECTIONS[@]}"; do
    run_one "${msg}_${cfg}_${be}__${dir}__${rel}" \
      --sweep --direction "$dir" \
      --message "$msg" --config "$cfg" --backend "$be" \
      --sweep-payloads "$SWEEP_PAYLOADS" --sweep-freqs "$SWEEP_FREQS" \
      --reliability "$rel" \
      --publish-jitter 0.01 --publish-jitter-seed 42 \
      --fill-encoding 64 --fill-frame-id 16 --fill-data-ratio 1.0 \
      --duration-per-step "$STEP_SEC"
  done
  done
done

echo "---- matrix driver finished: $i/$TOTAL attempted ----"
if [[ ${#FAILURES[@]} -gt 0 ]]; then
  echo "failures (${#FAILURES[@]}):"
  for name in "${FAILURES[@]}"; do
    echo "  $name"
  done
  exit 1
fi
echo "all invocations have .done markers in $RESULTS_DIR"
