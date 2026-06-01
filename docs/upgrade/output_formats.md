# NERDSS output format modernization

This page documents the first Phase 4 output-modernization slices: a run manifest schema, a small helper for inventorying existing outputs or writing `DATA/run_manifest.json`, and a C++ helper for writing standard CSV rows and manifest JSON. It is intentionally metadata-only. The current C++ simulation writers continue to emit the same legacy files and directory layout.

## Current legacy outputs

A typical serial run writes a `DATA` directory, or writes the same file names in the run directory in older examples. Common artifacts include:

| File | Current shape | Manifest role |
| --- | --- | --- |
| `copy_numbers_time.dat` | CSV-style time series with `Time (s)` header | `legacy_text_timeseries` |
| `observables_time.dat` | CSV-style time series when observables are configured | `legacy_text_timeseries` |
| `bound_pair_time.dat` | Tab-delimited time series for bound pair and event counts | `legacy_text_timeseries` |
| `mono_dimer_time.dat` | Tab-delimited monomer/dimer time series | `legacy_text_timeseries` |
| `histogram_complexes_time.dat` | Repeated `Time (s): ...` blocks followed by complex counts | `legacy_block_timeseries` |
| `event_counters_time.dat` | Repeated `time (s): ...` blocks | `legacy_block_timeseries` |
| `transition_matrix_time.dat` | Text blocks for transition matrices and lifetimes | `legacy_block_timeseries` |
| `assoc_dissoc_time.dat` | Text event stream for association and dissociation activity | `legacy_block_timeseries` |
| `smt_reactions_time.dat` | Text event stream for single-molecule reactions | `legacy_block_timeseries` |
| `trajectory.xyz` | XYZ trajectory | `trajectory_xyz` |
| `initial_crds.xyz` | Initial coordinates | `coordinate_xyz` |
| `system.psf` | PSF topology | `topology_psf` |
| `restart.dat` and `RESTARTS/*` | Restart state | `restart` |
| `rng_state` and rank-suffixed variants | Random-number generator state | `rng_state` |
| `PDB/*.pdb` and `PDB/*.cif` | Snapshot structures | `pdb_snapshot` |
| `OUTPUT` | Captured standard output in examples and notebooks | `stdout` |

MPI builds may also create rank-local files such as `DATA/copy_numbers_time_0.dat` before merge steps produce the rank-merged legacy file.

## Run manifest

The manifest is a JSON document that inventories one NERDSS run. It is meant to answer questions such as:

- Which output files were produced?
- Which legacy format family does each file belong to?
- Where are restart, trajectory, topology, snapshot, and log files?
- What file sizes, columns, approximate sample counts, and optional checksums are available?
- Which schema version did downstream tooling target?

The schema lives at:

```text
schemas/nerdss-run-manifest.schema.json
```

The schema version added in this slice is `1.0.0`, with `manifest_type` fixed to `nerdss-run-manifest`. Required top-level fields are:

- `schema_version`
- `manifest_type`
- `generated_at`
- `run`
- `files`

The `files` array is the central compatibility layer. Each entry has at least:

```json
{
  "path": "DATA/copy_numbers_time.dat",
  "role": "legacy_text_timeseries",
  "format": "csv",
  "exists": true,
  "size_bytes": 1234
}
```

Future phases can add structured schemas for individual output files while keeping this manifest as the stable inventory. Until then, `role` and `format` deliberately describe the current legacy text formats instead of promising normalized contents.

## C++ standard-format helpers

This slice adds a small, behavior-preserving helper under:

```text
include/io/standard_formats.hpp
src/io/standard_formats.cpp
```

The helper provides:

- `EscapeCsvField` and `WriteCsvRow` for RFC-4180-compatible quoting of comma,
  quote, newline, carriage-return, and empty fields.
- `BuildRunManifestJson` and `WriteRunManifestJson` for deterministic
  `nerdss-run-manifest` JSON text from already-known legacy output metadata.

The helper does not change any existing writer output. Existing `.dat`, `.xyz`,
`.psf`, restart, and PDB files remain the compatibility contract. The first
runtime adoption routes the legacy `copy_numbers_time.dat` header and data rows
and `observables_time.dat` data rows through `WriteCsvRow`, preserving the
current comma-separated bytes for ordinary names and numeric rows while
centralizing CSV row emission and quoting special header fields consistently.
The MPI adoption applies the same row helper to rank-local
`DATA/copy_numbers_time_<rank>.dat` rows and the merged
`mergeOUT/copy_numbers_time.dat` rows while keeping the legacy file names,
headers, and numeric row bytes unchanged. Future output modernization can
migrate additional writers or emit `DATA/run_manifest.json` from runtime
metadata without changing the legacy file names consumed by notebooks and
downstream scripts.

## Helper script

The helper is stdlib-only and lives at:

```text
python_scripts/inspect_nerdss_run_manifest.py
```

Usage:

```bash
python3 python_scripts/inspect_nerdss_run_manifest.py /path/to/run > run_manifest.json
```

Useful options:

```bash
python3 python_scripts/inspect_nerdss_run_manifest.py . --pretty
python3 python_scripts/inspect_nerdss_run_manifest.py DATA --run-id smoke-001
python3 python_scripts/inspect_nerdss_run_manifest.py . --hash --output run_manifest.json
python3 python_scripts/inspect_nerdss_run_manifest.py . --write-data-manifest --pretty
```

The helper scans the run directory, its `DATA`, `PDB`, and `RESTARTS` directories when present, and known root-level legacy files. It infers file role, file format, size, selected columns, approximate sample count, and MPI rank suffixes where possible. With `--write-data-manifest`, the helper writes the canonical manifest path for a run, `DATA/run_manifest.json`, creating `DATA` if needed.

Workflow or wrapper tooling can pass run metadata without changing the C++ simulation path:

```bash
python3 python_scripts/inspect_nerdss_run_manifest.py . \
  --write-data-manifest \
  --run-id smoke-001 \
  --status completed \
  --input-file parms.inp \
  --nerdss-executable ./nerdss \
  --nerdss-command "./nerdss -f parms.inp" \
  --nerdss-git-commit "$(git rev-parse HEAD)"
```

The output remains a best-effort manifest from existing artifacts. Missing simulation metadata such as executable version, exact input parameter expansion, and final status can be supplied by workflow tooling when available.

## Compatibility policy for this PR

This PR does not change C++ output behavior. It only adds:

- A schema for a future run manifest.
- Documentation describing the manifest and current legacy files.
- A Python helper that reads existing outputs and prints JSON metadata or writes
  `DATA/run_manifest.json`.
- A C++ helper for standard CSV row writing and run-manifest JSON generation.

Existing notebooks, validation data, and post-processing scripts should continue to consume the legacy files directly.
