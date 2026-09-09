#!/usr/bin/env bash
#
# Single spike run: cpp_to_cpp, constrained_pub_only, 1MB @ 10 Hz,
# shmem_ds, best-effort, full-bounds fill, seeded dither. Mirrors the
# ad-hoc single runs used during investigation, with the current driver
# defaults baked in. Same resumable convention as run_pilot.sh, plus a
# full analysis pass (tables, latency figures, report) afterwards.
#
# Usage (source tree, from the colcon workspace root):
#   ./src/constrained_pubsub_benchmark/scripts/run_spike.sh
#   RESULTS_DIR=/data/spike ./src/constrained_pubsub_benchmark/scripts/run_spike.sh
#   DRY_RUN=1 ./src/constrained_pubsub_benchmark/scripts/run_spike.sh  # list only
# Or installed (any directory, environment sourced):
#   ros2 run constrained_pubsub_benchmark spike
#

RESULTS_DIR="${RESULTS_DIR:-/tmp/spike_results}"
DRY_RUN="${DRY_RUN:-0}"
STEP_SEC=30
NAME="spike_cpp_to_cpp_1M_BE_10Hz"

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

mkdir -p "$RESULTS_DIR"
out="$RESULTS_DIR/$NAME.csv"
log="$RESULTS_DIR/$NAME.log"
done="$RESULTS_DIR/$NAME.done"
if [[ -f "$done" ]]; then
  echo "SKIP $NAME (already done, refreshing figures)"
else
  # Clear stale FastDDS SHM segments from unclean exits before measuring.
  rm -f /dev/shm/fast_datasharing_* /dev/shm/fastrtps_* 2>/dev/null || true
  echo "START $NAME"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "  python3 $BIN/inter_proc_benchmark.py --sweep --direction cpp_to_cpp ..."
    echo "  python3 $ANALYZE ..."
    exit 0
  fi
  python3 "$BIN/inter_proc_benchmark.py" \
    --sweep --direction cpp_to_cpp \
    --message exp --config constrained_pub_only --backend xcdr \
    --sweep-payloads 1M --sweep-freqs 10 --sweep-shm shmem_ds \
    --reliability best_effort \
    --publish-jitter 0.01 --publish-jitter-seed 42 \
    --fill-encoding 64 --fill-frame-id 16 --fill-data-ratio 1.0 \
    --duration-per-step "$STEP_SEC" > "$out.tmp" 2> "$log"
  rc=$?
  if [[ $rc -eq 0 ]]; then
    mv "$out.tmp" "$out"
    touch "$done"
    echo "DONE $NAME"
  else
    mv "$out.tmp" "$out.failed"
    echo "FAIL $NAME (rc=$rc, see $log)"
    exit 1
  fi
fi
echo "ANALYZE $NAME"
python3 "$ANALYZE" \
  "$RESULTS_DIR" --expected-jitter 0.01
rc=$?
if [[ $rc -eq 0 ]]; then
  echo "FIGURES $RESULTS_DIR/analysis"
else
  echo "ANALYZE FAIL (rc=$rc)"
  exit 1
fi
