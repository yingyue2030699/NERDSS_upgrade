#!/usr/bin/env python3
"""Run the standard NERDSS upgrade validation stack and write a JSON report."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, List, Optional


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_ROOT = REPO_ROOT / "validation_results"


@dataclass
class StepResult:
    name: str
    command: List[str]
    cwd: Path
    exit_code: Optional[int]
    runtime_seconds: float
    stdout_path: Path
    stderr_path: Path
    skipped: bool = False

    @property
    def passed(self) -> bool:
        return self.skipped or self.exit_code == 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "command": self.command,
            "cwd": str(self.cwd),
            "exit_code": self.exit_code,
            "runtime_seconds": round(self.runtime_seconds, 6),
            "stdout_path": str(self.stdout_path),
            "stderr_path": str(self.stderr_path),
            "skipped": self.skipped,
            "passed": self.passed,
        }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run smoke, unit, regression, and optional benchmark checks for "
            "NERDSS upgrade branches."
        )
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Directory for logs and validation_report.json.",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Reuse the existing bin/nerdss executable for smoke and regression.",
    )
    parser.add_argument(
        "--skip-smoke",
        action="store_true",
        help="Skip the serial smoke runner.",
    )
    parser.add_argument(
        "--skip-unit",
        action="store_true",
        help="Skip CMake unit tests.",
    )
    parser.add_argument(
        "--skip-regression",
        action="store_true",
        help="Skip regression validation.",
    )
    parser.add_argument(
        "--benchmarks",
        action="store_true",
        help="Run benchmark cases after correctness checks pass.",
    )
    parser.add_argument(
        "--benchmark-case",
        action="append",
        dest="benchmark_cases",
        help="Benchmark case id to run. May be repeated. Defaults to all cases.",
    )
    parser.add_argument(
        "--unit-build-dir",
        type=Path,
        default=REPO_ROOT / "build-upgrade-validation",
        help="CMake build directory for unit tests.",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=REPO_ROOT / "bin" / "nerdss",
        help="NERDSS serial executable path.",
    )
    parser.add_argument(
        "--fail-fast",
        action="store_true",
        help="Stop after the first failed non-skipped step.",
    )
    return parser.parse_args()


def timestamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")


def make_output_dir(path: Optional[Path]) -> Path:
    output_dir = path or DEFAULT_OUTPUT_ROOT / timestamp()
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir.resolve()


def run_command(name: str, command: List[str], cwd: Path, output_dir: Path) -> StepResult:
    stdout_path = output_dir / f"{name}.stdout.txt"
    stderr_path = output_dir / f"{name}.stderr.txt"
    started = time.monotonic()
    completed = subprocess.run(
        command,
        cwd=str(cwd),
        text=True,
        capture_output=True,
        check=False,
    )
    stdout_path.write_text(completed.stdout, encoding="utf-8")
    stderr_path.write_text(completed.stderr, encoding="utf-8")
    return StepResult(
        name=name,
        command=command,
        cwd=cwd,
        exit_code=completed.returncode,
        runtime_seconds=time.monotonic() - started,
        stdout_path=stdout_path,
        stderr_path=stderr_path,
    )


def skipped_step(name: str, output_dir: Path) -> StepResult:
    stdout_path = output_dir / f"{name}.stdout.txt"
    stderr_path = output_dir / f"{name}.stderr.txt"
    stdout_path.write_text("skipped\n", encoding="utf-8")
    stderr_path.write_text("", encoding="utf-8")
    return StepResult(
        name=name,
        command=[],
        cwd=REPO_ROOT,
        exit_code=0,
        runtime_seconds=0.0,
        stdout_path=stdout_path,
        stderr_path=stderr_path,
        skipped=True,
    )


def append_or_stop(
    steps: List[StepResult], result: StepResult, fail_fast: bool
) -> bool:
    steps.append(result)
    return fail_fast and not result.passed


def git_command(args: Iterable[str]) -> dict[str, Any]:
    command = ["git", *args]
    completed = subprocess.run(
        command,
        cwd=str(REPO_ROOT),
        text=True,
        capture_output=True,
        check=False,
    )
    return {
        "command": command,
        "exit_code": completed.returncode,
        "stdout": completed.stdout.strip(),
        "stderr": completed.stderr.strip(),
    }


def git_metadata() -> dict[str, Any]:
    commit = git_command(["rev-parse", "HEAD"])
    branch = git_command(["branch", "--show-current"])
    status = git_command(["status", "--short"])
    return {
        "commit": commit["stdout"] or None,
        "branch": branch["stdout"] or None,
        "dirty": bool(status["stdout"]),
        "status_short": status["stdout"].splitlines(),
    }


def write_report(output_dir: Path, steps: List[StepResult]) -> Path:
    report = {
        "schema_version": 1,
        "created_at": utc_now(),
        "repo": git_metadata(),
        "status": "passed" if all(step.passed for step in steps) else "failed",
        "steps": [step.to_dict() for step in steps],
    }
    path = output_dir / "validation_report.json"
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def print_summary(steps: List[StepResult], report_path: Path) -> None:
    for step in steps:
        status = "SKIP" if step.skipped else "PASS" if step.passed else "FAIL"
        print(f"{status} {step.name} ({step.runtime_seconds:.2f}s)")
    print(f"Report: {report_path}")


def main() -> int:
    args = parse_args()
    output_dir = make_output_dir(args.output_dir)
    steps: List[StepResult] = []

    if args.skip_smoke:
        if append_or_stop(steps, skipped_step("smoke", output_dir), args.fail_fast):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1
    else:
        smoke_command = [
            sys.executable,
            "-B",
            str(REPO_ROOT / "tools" / "run_smoke_tests.py"),
            "--artifact-dir",
            str(output_dir / "smoke"),
        ]
        if args.skip_build:
            smoke_command.extend(["--skip-build", "--executable", str(args.binary)])
        if append_or_stop(
            steps,
            run_command("smoke", smoke_command, REPO_ROOT, output_dir),
            args.fail_fast,
        ):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1

    if args.skip_unit:
        append_or_stop(steps, skipped_step("unit", output_dir), args.fail_fast)
    else:
        configure = run_command(
            "unit_configure",
            [
                "cmake",
                "-S",
                str(REPO_ROOT),
                "-B",
                str(args.unit_build_dir),
            ],
            REPO_ROOT,
            output_dir,
        )
        if append_or_stop(steps, configure, args.fail_fast):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1
        build = run_command(
            "unit_build",
            [
                "cmake",
                "--build",
                str(args.unit_build_dir),
                "--target",
                "nerdss_unit_tests",
            ],
            REPO_ROOT,
            output_dir,
        )
        if append_or_stop(steps, build, args.fail_fast):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1
        if append_or_stop(
            steps,
            run_command(
                "unit_ctest",
                ["ctest", "--test-dir", str(args.unit_build_dir), "--output-on-failure"],
                REPO_ROOT,
                output_dir,
            ),
            args.fail_fast,
        ):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1

    if args.skip_regression:
        append_or_stop(steps, skipped_step("regression", output_dir), args.fail_fast)
    else:
        regression_tmp = output_dir / "regression_tmp"
        regression_tmp.mkdir(parents=True, exist_ok=True)
        if append_or_stop(
            steps,
            run_command(
                "regression",
                [
                    sys.executable,
                    "-B",
                    str(REPO_ROOT / "tests" / "regression" / "run_regression.py"),
                    "--binary",
                    str(args.binary),
                    "--tmp-root",
                    str(regression_tmp),
                    "--json-output",
                    str(output_dir / "regression_report.json"),
                ],
                REPO_ROOT,
                output_dir,
            ),
            args.fail_fast,
        ):
            report_path = write_report(output_dir, steps)
            print_summary(steps, report_path)
            return 1

    if args.benchmarks:
        benchmark_command = [
            sys.executable,
            "-B",
            str(REPO_ROOT / "benchmarks" / "run_benchmarks.py"),
            "--nerdss",
            str(args.binary),
            "--output-dir",
            str(output_dir / "benchmarks"),
            "--format",
            "both",
        ]
        for case in args.benchmark_cases or []:
            benchmark_command.extend(["--case", case])
        append_or_stop(
            steps,
            run_command("benchmarks", benchmark_command, REPO_ROOT, output_dir),
            args.fail_fast,
        )
    else:
        append_or_stop(steps, skipped_step("benchmarks", output_dir), args.fail_fast)

    report_path = write_report(output_dir, steps)
    print_summary(steps, report_path)
    return 0 if all(step.passed for step in steps) else 1


if __name__ == "__main__":
    raise SystemExit(main())
