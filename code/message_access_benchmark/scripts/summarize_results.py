#!/usr/bin/env python3
# Copyright 2026 Ekumen Inc.
# SPDX-License-Identifier: Apache-2.0

"""Summarize Google Benchmark repetitions without mixing timing metrics."""

import argparse
import csv
import json
import math
import statistics
import sys
from collections import defaultdict

UNITS = {'ns': 1.0, 'us': 1_000.0, 'ms': 1_000_000.0, 's': 1_000_000_000.0}


def summarize(documents):
    """Return per-case medians, dispersion, and matched standard ratios."""
    samples = defaultdict(list)
    contexts = [
        {
            key: document.get('context', {}).get(key)
            for key in (
                'host_name', 'compiler', 'build_type', 'experimental', 'python', 'python_gc'
            )
        }
        for document in documents
    ]
    if any(context != contexts[0] for context in contexts[1:]):
        raise ValueError('cannot pool different build or machine contexts')
    for document in documents:
        for result in document['benchmarks']:
            if result.get('error_occurred'):
                raise ValueError(f"{result['name']}: {result.get('error_message')}")
            if result.get('run_type', 'iteration') != 'iteration':
                continue
            name = result.get('run_name', result['name'])
            parts = name.split('/')
            language, message, representation, operation = parts[:4]
            if language not in ('cpp', 'python'):
                raise ValueError(f'unknown benchmark language: {name}')
            metric = 'real_time' if language == 'python' else 'cpu_time'
            value = result[metric] * UNITS[result['time_unit']]
            if not math.isfinite(value) or value <= 0:
                raise ValueError(f'invalid timing for {name}')
            if message == 'MarkerArray':
                count = int(parts[4])
                context = ''
            else:
                count = int(message.removeprefix('Wide'))
                context = parts[4]
            key = (language, message, representation, operation, context, count, metric)
            samples[key].append(value)
    if not samples:
        raise ValueError('no iteration results; do not use aggregate-only JSON')
    medians = {key: statistics.median(values) for key, values in samples.items()}
    rows = []
    for key, values in sorted(samples.items()):
        language, message, representation, operation, context, count, metric = key
        median = medians[key]
        standard = (language, message, 'standard', operation, context, count, metric)
        ratio = median / medians[standard] if standard in medians else None
        rows.append(
            {
                'language': language,
                'message': message,
                'representation': representation,
                'operation': operation,
                'context': context,
                'count': count,
                'metric': metric,
                'repetitions': len(values),
                'median_ns': median,
                'mad_ns': statistics.median(abs(value - median) for value in values),
                'ns_per_item': median / count,
                'ratio_to_standard': ratio,
            }
        )
    return rows


def main():
    """Write a CSV summary while preserving raw JSON as the measurement record."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results', nargs='+', help='Google Benchmark JSON files')
    parser.add_argument('--output', required=True, help='Destination CSV')
    args = parser.parse_args()
    documents = []
    for path in args.results:
        with open(path, encoding='utf-8') as stream:
            documents.append(json.load(stream))
    rows = summarize(documents)
    if any(row['ratio_to_standard'] is None for row in rows):
        print('warning: some cases have no matching standard baseline', file=sys.stderr)
    if any(row['repetitions'] < 10 for row in rows):
        print('warning: some cases have fewer than 10 repetitions', file=sys.stderr)
    with open(args.output, 'w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


if __name__ == '__main__':
    main()
