# Message construction and access microbenchmarks

This package uses **Google Benchmark for every timed case**, including Python.
Python runs in an embedded interpreter; each Google Benchmark iteration calls a
Python function that traverses the whole message. There are no nodes, publishers,
serialization, DDS operations, or loans in the measured path.

The questions are:

- How long does creating and populating a 10,000-entry `MarkerArray` take?
- What changes when accessing owning experimental messages or external views?
- Can the C++ scalar wrappers inhibit optimization across thousands of named fields?

## Build

In the patched workspace:

```bash
cd ~/ws
MAKEFLAGS=-j2 colcon build --executor sequential \
  --packages-up-to message_access_benchmark \
  --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
```

Dependencies include Google Benchmark (`libbenchmark-dev`), Python development
headers, `pybind11_vendor`, and `visualization_msgs`. Experimental mode additionally
requires the patched C++ XCDR type support and `rosidl_runtime_cpython`.

The Docker `base_ros` and `patched_ros` targets build this package in standard-only
and experimental mode respectively. The development service mounts its source
alongside the other packages.

Run `code/import.sh` after updating this repository to apply the tracked
experimental sequence-runtime patch to the imported `rosidl` checkout. It fixes
allocation of message view objects for externally backed sequences; the
`patched_ros` Docker build applies the same patch automatically. Rebuild the
runtime and dependent generated messages, not just this benchmark.

For a stock ROS workspace with this package and its ordinary dependencies, build
only the standard cases:

```bash
MAKEFLAGS=-j2 colcon build --executor sequential \
  --packages-select message_access_benchmark \
  --cmake-args -DCMAKE_BUILD_TYPE=Release -DMESSAGE_ACCESS_EXPERIMENTAL=OFF
```

The experimental dependencies are deliberately not required by CMake in that mode.
If resolving dependencies with `rosdep` against stock ROS rather than the patched
source tree, skip the unavailable experimental keys:

```bash
rosdep install --from-paths src/message_access_benchmark --ignore-src -y \
  --skip-keys "rosidl_typesupport_xcdr_cpp rosidl_runtime_cpython"
```

## Workloads and timing boundaries

### MarkerArray (C++ and Python)

Counts: **1, 100, 1,000, 10,000**. Every marker is a cube with a unique ID, a
deterministic position, identity orientation, unit scale, visible color, and fixed
frame/namespace strings. Unused variable-length fields are empty. This is not a
point-cloud or mesh workload.

Representations:

- `standard`: ordinary generated messages.
- `experimental_owning`: default-constructed experimental messages.
- `experimental_external`: experimental messages constructed over aligned,
  application-owned storage using constrained XCDR type support. No owning
  fallback is permitted.

| Operation | Timed work |
| --- | --- |
| `fresh` | Allocate, construct, size, fill, and destroy a new message |
| `fill` | Fill an already-sized message, including strings |
| `read` | Traverse scalar members and consume a checksum |
| `write` | Update scalar members, excluding strings and resizing |
| `strings` | Assign the two fixed-length strings on every marker |
| `reconstruct` (C++ external only) | Destroy and reconstruct the view on an existing buffer, including initialization |

Constraint creation and layout caching are outside **all** timed loops. External
`fresh` includes backing allocation and view construction; `reconstruct` excludes
backing allocation. Access-only cases exclude all fixture construction, sizing,
initial fill, and destruction. The external layout reserves the two used strings
at their actual lengths and gives every unused variable-length field a zero bound.

Python uses public generated APIs: ordinary lists of markers for standard
messages and `markers.resize()` for experimental messages. Nested handles are
retained locally while operating on each marker, but traversal and property
access remain timed. External Python handles wrap the same C++ external fixture
using the generated message bridge; they never own that fixture. Python `fresh`
also includes creating and destroying that wrapper.

Python timing includes C++-to-Python calls and Python loop overhead. It is not a
measurement of a single C getter. CPU time is the primary metric for C++; wall
time is the primary metric for Python. Python cyclic GC is disabled for benchmark
runs; reference-counted destruction remains timed in `fresh`.

### Scalar-heavy C++ messages

CMake generates real ROS interfaces with **16, 256, 1,024, and 4,096 individually
named `float64` fields**, plus typed read, write, and read-modify-write kernels.
These are not array benchmarks.

Each representation has two contexts:

- `visible`: the kernel and fixture construction are visible in the benchmark
  translation unit.
- `opaque`: the same typed kernel is compiled in a separate translation unit.

Interprocedural optimization is disabled by this package. Do not inject `-flto`
through global compiler flags when comparing the two contexts. Writes use runtime
seeds; reads consume a checksum. Optimization barriers occur around complete
message operations, never around each scalar. The `view_bytes` counter describes
the C++ object, not its complete allocation footprint.

## Run

List cases or run a one-iteration smoke test:

```bash
ros2 run message_access_benchmark message_access --benchmark_list_tests=true
ros2 run message_access_benchmark message_access --benchmark_min_time=1x
```

Measure the headline case and save all repetitions:

```bash
mkdir -p results
taskset -c 2 ros2 run message_access_benchmark message_access \
  --benchmark_filter='(cpp|python)/MarkerArray/.*/(fresh|fill|read|write|strings)/10000' \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_min_time=0.5 \
  --benchmark_repetitions=10 \
  --benchmark_enable_random_interleaving=true \
  --benchmark_out=results/markers.json --benchmark_out_format=json
```

Run scalar cases by replacing the filter with `cpp/Wide`. Omit the filter for the
complete suite. A failing fixture or Python operation terminates the runner with
a nonzero exit status rather than silently selecting a different representation.

Google Benchmark JSON records timing units, repetitions, CPU information, and
additional compiler/build/Python/GC context. Also record source revisions (including
the imported core repositories), compiler flags, CPU affinity, power policy, and
any worktree changes alongside the results. Use the same settings for each build.
Run on an otherwise idle machine, one runner at a time; do not change host power
settings automatically.

Standard versus experimental **within the patched build** is the primary
comparison. Standard-only results from the baseline build are a separate control
for broader effects of the patches. Neither is a measurement of transport
performance.

## Summarize

```bash
ros2 run message_access_benchmark summarize results/markers.json \
  --output results/summary.csv
```

The CSV reports median nanoseconds per message, median absolute deviation,
nanoseconds per marker/field, repetition count, timing metric, and the ratio to a
matched standard case. A ratio above 1 means slower than standard. For the
headline result, divide median nanoseconds by 1,000,000 to obtain milliseconds per
10,000-marker message.

Multiple input JSON files may contain additional runs of the **same build and
machine configuration**. Do not pool baseline and patched build results. Keep raw
iteration rows: aggregate-only JSON cannot be summarized. Missing standard cases
and fewer than ten repetitions produce warnings. These descriptive statistics do
not establish statistical significance, individual-access tail latency, or a
portable performance guarantee.

## Correctness tests

```bash
colcon test --packages-select message_access_benchmark \
  --event-handlers console_direct+
colcon test-result --verbose
```

Tests check exactly 10,000 markers, persisted scalar/string assignments, checksums,
external backing addresses, Python mutations reaching the C++ backing, view
reconstruction, and the smallest/largest named-field kernels. Timing thresholds
are intentionally not enforced on shared CI runners.
