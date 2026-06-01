#!/usr/bin/env python3
"""Collect a small project coverage summary from gcov output."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


LINE_RE = re.compile(r"^Lines executed:([0-9.]+)% of ([0-9]+)$")
FILE_RE = re.compile(r"^File '(.+)'$")


@dataclass
class FileCoverage:
    path: str
    lines_total: int
    lines_covered: int
    percent: float

    def to_dict(self) -> dict[str, object]:
        return {
            "path": self.path,
            "lines_total": self.lines_total,
            "lines_covered": self.lines_covered,
            "percent": round(self.percent, 2),
        }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect line coverage for project files from a gcov build."
    )
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root.")
    parser.add_argument(
        "--build-dir",
        type=Path,
        required=True,
        help="Coverage build directory containing .gcda files.",
    )
    parser.add_argument(
        "--gcov",
        default="gcov",
        help="gcov-compatible executable. Default: gcov.",
    )
    parser.add_argument(
        "--include-prefix",
        action="append",
        default=["src/", "include/"],
        help=(
            "Project-relative file prefix to include. May be repeated. "
            "Default: src/ and include/."
        ),
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Path for JSON coverage summary.",
    )
    parser.add_argument(
        "--text-output",
        type=Path,
        help="Optional path for a human-readable text summary.",
    )
    parser.add_argument(
        "--fail-under",
        type=float,
        default=0.0,
        help="Fail if total line coverage is below this percent. Default: 0.",
    )
    return parser.parse_args()


def project_path(path_text: str, root: Path) -> str | None:
    path = Path(path_text)
    if not path.is_absolute():
        path = (root / path).resolve()
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return None


def should_include(path: str | None, prefixes: Iterable[str]) -> bool:
    if path is None:
        return False
    return any(path.startswith(prefix) for prefix in prefixes)


def run_gcov(gcov: str, gcda: Path, root: Path, output_dir: Path) -> str:
    command = [gcov, "-b", "-c", "-o", str(gcda.parent), str(gcda)]
    completed = subprocess.run(
        command,
        cwd=str(output_dir),
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"{' '.join(command)} failed with exit {completed.returncode}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )
    return completed.stdout


def parse_gcov_output(output: str, root: Path, prefixes: Iterable[str]) -> list[FileCoverage]:
    parsed: list[FileCoverage] = []
    current_path: str | None = None
    include_current = False

    for line in output.splitlines():
        file_match = FILE_RE.match(line)
        if file_match:
            current_path = project_path(file_match.group(1), root)
            include_current = should_include(current_path, prefixes)
            continue

        line_match = LINE_RE.match(line)
        if not line_match or not include_current or current_path is None:
            continue

        percent = float(line_match.group(1))
        lines_total = int(line_match.group(2))
        lines_covered = round(lines_total * percent / 100.0)
        parsed.append(
            FileCoverage(
                path=current_path,
                lines_total=lines_total,
                lines_covered=lines_covered,
                percent=percent,
            )
        )
        current_path = None
        include_current = False

    return parsed


def merge_file_coverage(files: Iterable[FileCoverage]) -> list[FileCoverage]:
    by_path: dict[str, FileCoverage] = {}
    for item in files:
        existing = by_path.get(item.path)
        if existing is None or item.lines_total > existing.lines_total:
            by_path[item.path] = item
    return sorted(by_path.values(), key=lambda entry: entry.path)


def write_text_summary(path: Path, total: dict[str, object], files: list[FileCoverage]) -> None:
    lines = [
        f"Total line coverage: {total['percent']:.2f}%",
        f"Covered lines: {total['lines_covered']} / {total['lines_total']}",
        f"Files included: {len(files)}",
        "",
        "Lowest-covered files:",
    ]
    for item in sorted(files, key=lambda entry: entry.percent)[:20]:
        lines.append(
            f"{item.percent:6.2f}%  {item.lines_covered:5d}/{item.lines_total:<5d}  {item.path}"
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    build_dir = args.build_dir.resolve()

    gcov = shutil.which(args.gcov)
    if gcov is None:
        print(f"error: gcov executable not found: {args.gcov}", file=sys.stderr)
        return 2

    gcda_files = sorted(build_dir.rglob("*.gcda"))
    if not gcda_files:
        print(f"error: no .gcda files found under {build_dir}", file=sys.stderr)
        return 2

    collected: list[FileCoverage] = []
    with tempfile.TemporaryDirectory(prefix="nerdss-gcov-") as gcov_output_dir:
        gcov_output_path = Path(gcov_output_dir)
        for gcda in gcda_files:
            output = run_gcov(gcov, gcda, root, gcov_output_path)
            collected.extend(parse_gcov_output(output, root, args.include_prefix))

    files = merge_file_coverage(collected)
    lines_total = sum(item.lines_total for item in files)
    lines_covered = sum(item.lines_covered for item in files)
    percent = (lines_covered / lines_total * 100.0) if lines_total else 0.0
    total = {
        "lines_total": lines_total,
        "lines_covered": lines_covered,
        "percent": percent,
    }

    report = {
        "schema_version": 1,
        "tool": "gcov",
        "gcov_executable": gcov,
        "build_dir": str(build_dir),
        "include_prefixes": args.include_prefix,
        "total": {
            "lines_total": lines_total,
            "lines_covered": lines_covered,
            "percent": round(percent, 2),
        },
        "files": [item.to_dict() for item in files],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.text_output:
        write_text_summary(args.text_output, total, files)

    print(
        f"Total line coverage: {percent:.2f}% "
        f"({lines_covered}/{lines_total} project lines)"
    )
    return 1 if percent < args.fail_under else 0


if __name__ == "__main__":
    raise SystemExit(main())
