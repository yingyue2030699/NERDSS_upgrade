#!/usr/bin/env python3

import argparse
import subprocess
import tempfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build and run focused I/O standard-format tests.")
    parser.add_argument("--cxx", default="c++", help="C++ compiler command.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="nerdss-io-format-test-") as tmp_dir:
        test_binary = Path(tmp_dir) / "test_standard_formats"
        compile_cmd = [
            args.cxx,
            "-std=c++11",
            "-I",
            str(repo_root / "include"),
            str(repo_root / "tests/io/test_standard_formats.cpp"),
            str(repo_root / "src/io/standard_formats.cpp"),
            str(repo_root / "src/io/write_all_species.cpp"),
            str(repo_root / "src/io/write_observables.cpp"),
            "-o",
            str(test_binary),
        ]
        subprocess.run(compile_cmd, cwd=repo_root, check=True)
        subprocess.run([str(test_binary), tmp_dir], cwd=repo_root, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
