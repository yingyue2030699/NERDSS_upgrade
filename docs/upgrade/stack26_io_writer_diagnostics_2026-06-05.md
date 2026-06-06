# Stack 26 I/O Writer Diagnostics - 2026-06-05

## Scope

This diagnostics slice adds structured open-failure reporting to remaining
serial core I/O writers that own their output streams.

- Checks `DATA/system.psf` before writing PSF data.
- Checks full-system XYZ outputs before writing coordinate data.
- Checks association-debug XYZ outputs before writing coordinate data.
- Checks error/debug coordinate dump files before writing per-molecule CRD data.
- Uses `nerdss::io::WriteArtifactWriteOpenDiagnostic` and returns or continues
  before writing through a bad stream. This mirrors the existing PDB and bonded
  complex JSON writer behavior for optional artifacts.

## Local Validation

Commands run on `codex/io-writer-open-diagnostics-stack26-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh src/io/write_psf.cpp src/io/write_xyz.cpp src/io/write_xyz_assoc.cpp src/io/write_crds.cpp src/io/write_complex_crds.cpp`:
  exited 0 with existing legacy writer warnings.
- `cmake -S . -B build-io-writer-open-diagnostics`: passed.
- `cmake --build build-io-writer-open-diagnostics --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-io-writer-open-diagnostics --output-on-failure`:
  passed, 3/3 tests passed.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-io-writer-open-diagnostics-validation`:
  passed. Benchmarks were skipped for this narrow diagnostics slice.

## Follow-Up Notes

- MPI-owned writers and merge-output paths remain separate because local MPI
  rebuild is still blocked by the `mpicxx` wrapper configuration.
- Fatal executable-owned streams should continue to use `ExitWithDiagnostic`
  rather than this optional-artifact return pattern.
