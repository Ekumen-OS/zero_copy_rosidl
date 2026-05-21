#!/usr/bin/env python3
"""Parse a performance_test JSON log file and print key metrics."""

import argparse
import json
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json_file", help="Path to the performance_test JSON log file")
    args = parser.parse_args()

    with open(args.json_file) as f:
        data = json.load(f)

    exps = data.get("analysis_results", [])
    last = exps[-1] if exps else data

    print(
        f"recv={last.get('num_samples_received', '?')} "
        f"sent={last.get('num_samples_sent', '?')} "
        f"lost={last.get('num_samples_lost', '?')}"
    )
    print(
        f"latency "
        f"min={last.get('latency_min', float('nan')):.6f} "
        f"max={last.get('latency_max', float('nan')):.6f} "
        f"mean={last.get('latency_mean', float('nan')):.6f} "
        f"var={last.get('latency_variance', float('nan')):.6f}"
    )


if __name__ == "__main__":
    main()
