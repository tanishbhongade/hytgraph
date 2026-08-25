#!/usr/bin/env python3
"""Phase 0 experiment runner skeleton.

This runner deliberately does not implement any HyTGraph algorithm. It loads a
YAML experiment configuration, records reproducibility metadata, and invokes
the Phase 0 dummy executable to produce a structured JSON result.
"""

from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

try:
    import yaml
except ImportError as exc:  # pragma: no cover - environment-specific
    raise SystemExit("PyYAML is required to run experiments: %s" % exc)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a HyTGraph experiment skeleton")
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executable", required=True, type=Path)
    return parser.parse_args()


def git_commit() -> str:
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        )
        return completed.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "uncommitted"


def main() -> int:
    args = parse_args()
    with args.config.open("r", encoding="utf-8") as handle:
        config = yaml.safe_load(handle) or {}

    experiment_id = str(config.get("experiment_id", "phase0-dummy"))
    algorithm = str(config.get("algorithm", "unspecified"))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    command = [
        str(args.executable),
        "--output",
        str(args.output),
        "--experiment-id",
        experiment_id,
        "--algorithm",
        algorithm,
    ]

    for key, value in sorted(config.items()):
        if isinstance(value, (str, int, float, bool)):
            command.extend(["--config", f"{key}={str(value).lower() if isinstance(value, bool) else value}"])

    subprocess.run(command, check=True)

    with args.output.open("r", encoding="utf-8") as handle:
        result = json.load(handle)

    result["git_commit"] = git_commit()
    result["runner"] = {
        "name": "experiments/run_experiment.py",
        "python_version": platform.python_version(),
        "platform": platform.platform(),
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
    }
    result["experiment_config"] = config

    with args.output.open("w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
        handle.write("\n")

    print(f"Experiment result: {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
