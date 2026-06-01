# Static Analysis And Sanitizer Backlog

This backlog records current findings from the Phase 2 sanitizer/static-analysis
guardrail setup. Items here are intentionally not fixed in the tooling PR unless
the fix is limited to build configuration or documentation.

## Open Items

### P1: Serial CMake build fails on undeclared bonded-complex JSON symbols

- Reproduction:
  `cmake --build build/validate-asan --target nerdss --parallel 4`
- Observed on: 2026-05-28, `codex/upgrade-sanitizer-static` based on
  `origin/master`.
- Failure:
  - `EXEs/nerdss.cpp` references `params.bondedComplexWrite`, but
    `include/classes/class_Parameters.hpp` on `origin/master` does not declare
    that member.
  - `EXEs/nerdss.cpp` calls `write_bonded_complex_json`, but
    `include/io/io.hpp` on `origin/master` does not declare that function.
- Impact:
  serial sanitizer builds configure correctly but cannot complete until the
  source/header mismatch is resolved.
- Notes:
  `origin/codex/upgrade-smoke-runner` appears to add the
  `Parameters::bondedComplexWrite` declaration. Coordinate with that branch
  rather than duplicating source/header fixes in this tooling-only PR.

### Resolved 2026-06-01: local clang-tidy discovery

- Reproduction: `tools/run_static_analysis.sh EXEs/nerdss.cpp`
- Previous failure: `clang-tidy was not found on PATH.`
- Resolution:
  `tools/run_static_analysis.sh` now honors `CLANG_TIDY_BIN`, checks common
  Homebrew LLVM install locations, and passes the active macOS SDK/libc++ paths
  to clang-tidy.
- Remaining work:
  run focused static-analysis cleanup slices over `src/math`, `src/parser`, and
  `EXEs` to reduce the existing warning backlog.

### P3: CMake 4.3 warns about compatibility policy floor

- Reproduction:
  `cmake -S . -B build/validate-asan -DCMAKE_BUILD_TYPE=Debug -DNERDSS_ENABLE_ASAN=ON`
- Warning:
  CMake 4.3 warns that compatibility with CMake versions older than 3.10 will be
  removed in a future CMake release.
- Impact:
  configure succeeds after raising the repository floor from 3.0 to 3.5, but a
  future upgrade should decide whether NERDSS can require CMake 3.10 or newer.
- Suggested next step:
  include the CMake minimum in Agent A's environment policy decision.
