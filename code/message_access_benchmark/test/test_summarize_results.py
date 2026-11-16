# Copyright 2026 Ekumen Inc.
# SPDX-License-Identifier: Apache-2.0

"""Focused tests of timing normalization and representation matching."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from summarize_results import summarize  # noqa: E402


def sample(name, cpu, real, unit='us', **extra):
    """Build the subset of a Google Benchmark iteration used by analysis."""
    return dict(name=name, cpu_time=cpu, real_time=real, time_unit=unit, **extra)


def test_matched_ratios_use_language_specific_metric():
    """Match storage modes at the same size without mixing CPU and wall time."""
    rows = summarize(
        [
            {
                'benchmarks': [
                    sample('cpp/MarkerArray/standard/read/10000', 2, 20),
                    sample('cpp/MarkerArray/standard/read/10000', 4, 40),
                    sample('cpp/MarkerArray/experimental_external/read/10000', 6, 90),
                    sample('python/MarkerArray/standard/read/10000', 1, 10),
                    sample('python/MarkerArray/experimental_owning/read/10000', 20, 20),
                    sample('cpp/Wide16/standard/read/opaque', 100, 100, 'ns'),
                ]
            }
        ]
    )
    indexed = {(row['language'], row['representation'], row['message']): row for row in rows}
    baseline = indexed['cpp', 'standard', 'MarkerArray']
    assert baseline['median_ns'] == 3000
    assert baseline['mad_ns'] == 1000
    assert baseline['ns_per_item'] == 0.3
    assert indexed['cpp', 'experimental_external', 'MarkerArray']['ratio_to_standard'] == 2
    assert indexed['python', 'experimental_owning', 'MarkerArray']['ratio_to_standard'] == 2
    assert indexed['cpp', 'standard', 'Wide16']['context'] == 'opaque'


def test_aggregates_not_counted_as_repetitions():
    """Ignore aggregate rows and normalize repeated-run names."""
    rows = summarize(
        [
            {
                'benchmarks': [
                    sample(
                        'cpp/Wide16/standard/read/visible/repeats:10',
                        3,
                        4,
                        run_name='cpp/Wide16/standard/read/visible',
                    ),
                    sample(
                        'cpp/Wide16/standard/read/visible_mean',
                        3,
                        4,
                        run_type='aggregate',
                    ),
                ]
            }
        ]
    )
    assert rows[0]['repetitions'] == 1


def test_errors_and_empty_results_fail_explicitly():
    """Never turn a failed or empty run into a successful-looking report."""
    with pytest.raises(ValueError, match='storage unavailable'):
        summarize(
            [
                {
                    'benchmarks': [
                        {
                            'name': 'failed',
                            'error_occurred': True,
                            'error_message': 'storage unavailable',
                        }
                    ]
                }
            ]
        )
    with pytest.raises(ValueError, match='no iteration results'):
        summarize([{'benchmarks': []}])


def test_different_builds_are_not_pooled():
    """Keep the standard-only control separate from the patched comparison."""
    with pytest.raises(ValueError, match='different build or machine'):
        summarize(
            [
                {'context': {'experimental': 'enabled'}, 'benchmarks': []},
                {'context': {'experimental': 'disabled'}, 'benchmarks': []},
            ]
        )
