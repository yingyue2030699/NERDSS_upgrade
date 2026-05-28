#!/usr/bin/env python3
"""Inspect legacy NERDSS outputs and emit or write a run manifest."""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import hashlib
import json
import re
import shlex
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "1.0.0"
GENERATOR_VERSION = "0.2.0"

KNOWN_FILES: dict[str, dict[str, str]] = {
    "copy_numbers_time.dat": {
        "role": "legacy_text_timeseries",
        "format": "csv",
        "description": "Copy numbers for molecules and bound products over time.",
        "time_units": "s",
    },
    "observables_time.dat": {
        "role": "legacy_text_timeseries",
        "format": "csv",
        "description": "Configured observables over time.",
        "time_units": "s",
    },
    "bound_pair_time.dat": {
        "role": "legacy_text_timeseries",
        "format": "tsv",
        "description": "Bound pair counts and association event counters over time.",
        "time_units": "s",
    },
    "mono_dimer_time.dat": {
        "role": "legacy_text_timeseries",
        "format": "tsv",
        "description": "Monomer and dimer counts by molecule type over time.",
        "time_units": "s",
    },
    "histogram_complexes_time.dat": {
        "role": "legacy_block_timeseries",
        "format": "legacy_text",
        "description": "Repeated time blocks of complex composition counts.",
        "time_units": "s",
    },
    "event_counters_time.dat": {
        "role": "legacy_block_timeseries",
        "format": "legacy_text",
        "description": "Repeated time blocks of event counters.",
        "time_units": "s",
    },
    "transition_matrix_time.dat": {
        "role": "legacy_block_timeseries",
        "format": "legacy_text",
        "description": "Transition matrix and lifetime text blocks.",
        "time_units": "s",
    },
    "assoc_dissoc_time.dat": {
        "role": "legacy_block_timeseries",
        "format": "legacy_text",
        "description": "Association and dissociation event stream.",
        "time_units": "s",
    },
    "smt_reactions_time.dat": {
        "role": "legacy_block_timeseries",
        "format": "legacy_text",
        "description": "Single-molecule reaction event stream.",
        "time_units": "s",
    },
    "trajectory.xyz": {
        "role": "trajectory_xyz",
        "format": "xyz",
        "description": "XYZ trajectory.",
    },
    "initial_crds.xyz": {
        "role": "coordinate_xyz",
        "format": "xyz",
        "description": "Initial molecule coordinates.",
    },
    "system.psf": {
        "role": "topology_psf",
        "format": "psf",
        "description": "PSF topology.",
    },
    "restart.dat": {
        "role": "restart",
        "format": "restart",
        "description": "Restart state.",
    },
    "rng_state": {
        "role": "rng_state",
        "format": "unknown",
        "description": "Random-number generator state.",
    },
    "OUTPUT": {
        "role": "stdout",
        "format": "legacy_text",
        "description": "Captured NERDSS standard output.",
    },
    "run_manifest.json": {
        "role": "manifest",
        "format": "json",
        "description": "NERDSS run manifest.",
    },
}

RANK_SUFFIX_RE = re.compile(r"^(?P<stem>.+)_(?P<rank>[0-9]+)(?P<suffix>\.[^.]+)$")
TIME_BLOCK_RE = re.compile(r"^time\s*\(s\)\s*:", re.IGNORECASE)


def utc_now() -> str:
    return _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat()


def relative_path(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_datetime_arg(value: str) -> str:
    """Return an ISO-8601 datetime string acceptable to the manifest schema."""
    if value.endswith("Z"):
        value = f"{value[:-1]}+00:00"
    try:
        parsed = _dt.datetime.fromisoformat(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            f"expected ISO-8601 datetime, got {value!r}"
        ) from exc
    if parsed.tzinfo is None:
        raise argparse.ArgumentTypeError(
            f"expected timezone-aware ISO-8601 datetime, got {value!r}"
        )
    return parsed.isoformat()


def parse_command(value: str) -> list[str]:
    try:
        command = shlex.split(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            f"could not parse command line {value!r}: {exc}"
        ) from exc
    if not command:
        raise argparse.ArgumentTypeError("command line must not be empty")
    return command


def parse_build_metadata(value: str) -> dict[str, str | int | float | bool | None]:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise argparse.ArgumentTypeError(
            f"expected JSON object for build metadata: {exc}"
        ) from exc
    if not isinstance(parsed, dict):
        raise argparse.ArgumentTypeError("build metadata must be a JSON object")

    allowed_types = (str, int, float, bool, type(None))
    for key, item in parsed.items():
        if not isinstance(key, str):
            raise argparse.ArgumentTypeError("build metadata keys must be strings")
        if not isinstance(item, allowed_types):
            raise argparse.ArgumentTypeError(
                "build metadata values must be strings, numbers, booleans, or null"
            )
    return parsed


def base_name_for_ranked_file(name: str) -> tuple[str, int | None]:
    match = RANK_SUFFIX_RE.match(name)
    if not match:
        return name, None

    base_name = f"{match.group('stem')}{match.group('suffix')}"
    if base_name in KNOWN_FILES:
        return base_name, int(match.group("rank"))
    return name, None


def classify_file(path: Path) -> dict[str, Any]:
    base_name, rank = base_name_for_ranked_file(path.name)
    details: dict[str, Any] = dict(KNOWN_FILES.get(base_name, {}))

    suffix = path.suffix.lower()
    if not details:
        if suffix == ".pdb":
            details = {"role": "pdb_snapshot", "format": "pdb"}
        elif suffix == ".cif":
            details = {"role": "pdb_snapshot", "format": "cif"}
        elif suffix == ".xyz":
            details = {"role": "trajectory_xyz", "format": "xyz"}
        elif suffix == ".psf":
            details = {"role": "topology_psf", "format": "psf"}
        elif suffix == ".json":
            details = {"role": "other", "format": "json"}
        else:
            details = {"role": "other", "format": "unknown"}

    if rank is not None:
        details["generated_by_rank"] = rank

    return details


def read_text_prefix(path: Path, limit: int = 65536) -> str:
    with path.open("rb") as handle:
        data = handle.read(limit)
    return data.decode("utf-8", errors="replace")


def infer_columns_and_samples(path: Path, file_format: str) -> dict[str, Any]:
    if file_format in {"binary", "unknown", "restart", "pdb", "cif", "psf"}:
        return {}

    try:
        text = read_text_prefix(path)
    except OSError:
        return {}

    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if not lines:
        return {"sample_count": 0}

    metadata: dict[str, Any] = {}
    first = lines[0]
    if file_format == "tsv" or "\t" in first:
        metadata["columns"] = [part for part in first.split("\t") if part]
        metadata["sample_count"] = max(len(lines) - 1, 0)
    elif file_format == "csv" or "," in first:
        metadata["columns"] = next(csv.reader([first]))
        metadata["sample_count"] = max(len(lines) - 1, 0)
    else:
        block_count = sum(
            1
            for line in lines
            if TIME_BLOCK_RE.match(line) or line.lower().startswith("time:")
        )
        if block_count:
            metadata["sample_count"] = block_count

    return metadata


def collect_files(run_dir: Path, exclude_paths: set[Path] | None = None) -> list[Path]:
    exclude_paths = exclude_paths or set()
    scan_roots: list[Path] = []
    if run_dir.name in {"DATA", "PDB", "RESTARTS"}:
        scan_roots.append(run_dir)
    else:
        scan_roots.append(run_dir)
        for child_name in ("DATA", "PDB", "RESTARTS"):
            child = run_dir / child_name
            if child.is_dir():
                scan_roots.append(child)

    files: list[Path] = []
    seen: set[Path] = set()
    for root in scan_roots:
        for path in sorted(root.iterdir()):
            if not path.is_file() or path in seen:
                continue
            if path.resolve() in exclude_paths:
                continue
            if root == run_dir and path.name not in KNOWN_FILES:
                continue
            seen.add(path)
            files.append(path)
    return files


def collect_directories(run_dir: Path, manifest_root: Path) -> list[dict[str, Any]]:
    roles = {
        "DATA": "data",
        "PDB": "pdb_snapshots",
        "RESTARTS": "restarts",
    }
    directories: list[dict[str, Any]] = []
    for name, role in roles.items():
        path = run_dir / name
        if path.is_dir():
            directories.append(
                {
                    "path": relative_path(path, manifest_root),
                    "role": role,
                    "exists": True,
                    "file_count": sum(1 for child in path.iterdir() if child.is_file()),
                }
            )
    return directories


def build_manifest(args: argparse.Namespace) -> dict[str, Any]:
    run_dir = args.run_directory.resolve()
    manifest_root = run_dir.parent if run_dir.name in {"DATA", "PDB", "RESTARTS"} else run_dir

    files = []
    for path in collect_files(run_dir, args.exclude_paths):
        entry: dict[str, Any] = {
            "path": relative_path(path, manifest_root),
            "exists": True,
            "size_bytes": path.stat().st_size,
        }
        entry.update(classify_file(path))
        entry.update(infer_columns_and_samples(path, entry.get("format", "unknown")))
        if args.hash:
            entry["sha256"] = sha256_file(path)
        files.append(entry)

    run: dict[str, Any] = {
        "output_directory": relative_path(run_dir, manifest_root),
        "working_directory": args.working_directory or str(run_dir),
        "status": args.status,
    }
    data_dir = run_dir / "DATA"
    if data_dir.is_dir():
        run["data_directory"] = relative_path(data_dir, manifest_root)
    if args.run_id:
        run["id"] = args.run_id
    if args.title:
        run["title"] = args.title
    if args.input_file:
        run["input_file"] = args.input_file
    if args.started_at:
        run["started_at"] = args.started_at
    if args.ended_at:
        run["ended_at"] = args.ended_at
    if args.mpi_ranks is not None:
        run["mpi"] = {
            "enabled": args.mpi_ranks > 1,
            "ranks": args.mpi_ranks,
        }

    manifest: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "manifest_type": "nerdss-run-manifest",
        "generated_at": utc_now(),
        "generator": {
            "name": "inspect_nerdss_run_manifest.py",
            "version": GENERATOR_VERSION,
            "command": sys.argv,
        },
        "run": run,
        "files": files,
        "notes": [
            "Generated from existing legacy outputs; simulation metadata may be incomplete."
        ],
    }

    nerdss: dict[str, Any] = {}
    if args.nerdss_executable:
        nerdss["executable"] = args.nerdss_executable
    if args.nerdss_command:
        nerdss["command"] = args.nerdss_command
    if args.nerdss_version:
        nerdss["version"] = args.nerdss_version
    if args.nerdss_git_commit:
        nerdss["git_commit"] = args.nerdss_git_commit
    if args.nerdss_build:
        nerdss["build"] = args.nerdss_build
    if nerdss:
        manifest["nerdss"] = nerdss

    directories = collect_directories(run_dir, manifest_root)
    if directories:
        manifest["directories"] = directories

    return manifest


def default_data_manifest_path(run_directory: Path) -> Path:
    if run_directory.name == "DATA":
        return run_directory / "run_manifest.json"
    if run_directory.name in {"PDB", "RESTARTS"}:
        return run_directory.parent / "DATA" / "run_manifest.json"
    return run_directory / "DATA" / "run_manifest.json"


def add_manifest_file_entry(
    manifest: dict[str, Any], output_path: Path, run_directory: Path
) -> None:
    run_dir = run_directory.resolve()
    manifest_root = run_dir.parent if run_dir.name in {"DATA", "PDB", "RESTARTS"} else run_dir
    manifest_path = relative_path(output_path.resolve(), manifest_root)
    if any(entry.get("path") == manifest_path for entry in manifest["files"]):
        return
    manifest["files"].append(
        {
            "path": manifest_path,
            "role": "manifest",
            "format": "json",
            "exists": True,
            "description": "NERDSS run manifest generated after inspecting legacy outputs.",
            "notes": [
                "Size and checksum are omitted to avoid circular manifest metadata."
            ],
        }
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Inspect a NERDSS run directory or DATA directory and emit or write a run manifest."
    )
    parser.add_argument(
        "run_directory",
        nargs="?",
        default=Path("."),
        type=Path,
        help="Run directory to inspect, or a DATA/PDB/RESTARTS directory.",
    )
    parser.add_argument(
        "--run-id",
        help="Optional run identifier to include in the manifest.",
    )
    parser.add_argument(
        "--title",
        help="Optional human-readable run title.",
    )
    parser.add_argument(
        "--input-file",
        help="Optional input file path to record in run.input_file.",
    )
    parser.add_argument(
        "--working-directory",
        help="Working directory to record for the run. Defaults to the inspected directory.",
    )
    parser.add_argument(
        "--status",
        choices=["planned", "running", "completed", "failed", "unknown"],
        default="unknown",
        help="Run status to record. Defaults to unknown.",
    )
    parser.add_argument(
        "--started-at",
        type=parse_datetime_arg,
        help="Run start time as a timezone-aware ISO-8601 timestamp.",
    )
    parser.add_argument(
        "--ended-at",
        type=parse_datetime_arg,
        help="Run end time as a timezone-aware ISO-8601 timestamp.",
    )
    parser.add_argument(
        "--mpi-ranks",
        type=int,
        help="Number of MPI ranks used for the run.",
    )
    parser.add_argument(
        "--nerdss-executable",
        help="NERDSS executable path or name used for the run.",
    )
    parser.add_argument(
        "--nerdss-command",
        type=parse_command,
        help="NERDSS command line used for the run, parsed with shell-style quoting.",
    )
    parser.add_argument(
        "--nerdss-version",
        help="NERDSS version string to record, if known.",
    )
    parser.add_argument(
        "--nerdss-git-commit",
        help="NERDSS source git commit to record, if known.",
    )
    parser.add_argument(
        "--nerdss-build",
        type=parse_build_metadata,
        help="NERDSS build metadata as a JSON object with scalar values.",
    )
    parser.add_argument(
        "--hash",
        action="store_true",
        help="Compute sha256 for each discovered file.",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty-print JSON output.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Write JSON to this file instead of standard output.",
    )
    parser.add_argument(
        "--write-data-manifest",
        action="store_true",
        help="Write to DATA/run_manifest.json for the inspected run.",
    )
    args = parser.parse_args(argv)
    if args.mpi_ranks is not None and args.mpi_ranks < 1:
        parser.error("--mpi-ranks must be at least 1")
    if args.output and args.write_data_manifest:
        parser.error("--output and --write-data-manifest are mutually exclusive")
    args.exclude_paths = set()
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if not args.run_directory.exists():
        print(f"error: path does not exist: {args.run_directory}", file=sys.stderr)
        return 2
    if not args.run_directory.is_dir():
        print(f"error: expected a directory: {args.run_directory}", file=sys.stderr)
        return 2

    output_path: Path | None = None
    if args.write_data_manifest:
        output_path = default_data_manifest_path(args.run_directory).resolve()
    elif args.output:
        output_path = args.output.resolve()

    if output_path is not None:
        args.exclude_paths.add(output_path)

    manifest = build_manifest(args)
    if args.write_data_manifest and output_path is not None:
        add_manifest_file_entry(manifest, output_path, args.run_directory)

    json_text = json.dumps(
        manifest,
        indent=2 if args.pretty else None,
        sort_keys=True,
    )
    json_text += "\n"

    if output_path is not None:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json_text, encoding="utf-8")
    else:
        print(json_text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
