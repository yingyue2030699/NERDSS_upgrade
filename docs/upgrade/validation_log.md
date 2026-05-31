# Upgrade Validation Log

Record validation runs for upgrade work in reverse chronological order. Keep
entries concise, reproducible, and explicit about skipped checks.

## Entry Template

Date:

Branch:

Commit:

Workstream:

Environment:
- OS:
- Compiler:
- GSL:
- CMake:
- Make:
- MPI:

Commands:

Results:

Artifacts:

Notes:

## 2026-05-28: Agent A Phase 0 Environment Baseline

Date: 2026-05-28

Branch: `codex/upgrade-baseline-policy`

Commit: Pending at initial log creation

Workstream: Phase 0 repository baseline and branch policy

Environment:
- OS: macOS 14.5, build 23F79, arm64
- Compiler: Apple clang 16.0.0 via `g++` and `clang++`
- GSL: 2.8 from Homebrew, headers in `/opt/homebrew/Cellar/gsl/2.8/include`
- CMake: Not available on `PATH` in this environment
- Make: GNU Make 3.81
- MPI: MPICH/HYDRA 3.3.2 runtime present through Anaconda; `mpicxx --version`
  fails because the configured `x86_64-apple-darwin13.4.0-clang++` wrapper
  compiler is not available

Commands:
```sh
sw_vers
uname -m
g++ --version
clang++ --version
gsl-config --version
gsl-config --cflags
gsl-config --libs
cmake --version
make --version
mpicxx --version
mpirun --version
git status --short --branch
make serial
```

Results:
- Serial compiler and GSL dependencies are available.
- `make serial` compiles object files but fails when compiling
  `EXEs/nerdss.cpp` because the current baseline references
  `Parameters::bondedComplexWrite` and `write_bonded_complex_json` without
  visible declarations.
- CMake validation is blocked until CMake is installed or added to `PATH`.
- MPI build validation is blocked until the MPI compiler wrapper points to an
  available compiler.
- Repository worktree started clean on `codex/upgrade-baseline-policy`.

Artifacts:
- This log entry.

Notes:
- This Phase 0 branch is documentation and metadata only. It does not modify
  build logic or simulation source code.
- The serial build failure is recorded as a baseline blocker for follow-up by
  the build or smoke-runner workstream.

## 2026-05-30: Diagnostics Test Registration

Date: 2026-05-30

Branch: `codex/diagnostics-test-registration`

Commit: Pending at validation time

Workstream: Diagnostics and error-boundary follow-up

Environment:
- OS: macOS/Darwin arm64 local Codex workspace
- Compiler: AppleClang 16 via CMake and Apple clang via `g++`
- GSL: Homebrew GSL from existing project configuration
- CMake: Available on `PATH`
- Make: Available on `PATH`
- MPI: Not exercised by this serial diagnostics slice

Commands:
```sh
cmake -S . -B build-upgrade-validation
cmake --build build-upgrade-validation --target clean
cmake --build build-upgrade-validation --target nerdss_unit_tests
ctest --test-dir build-upgrade-validation --output-on-failure
make serial
tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-diagnostics-test-registration-validation
```

Results:
- CMake configure passed.
- Building the existing `nerdss_unit_tests` target from a cleaned CMake tree
  also built the `nerdss_diagnostics_tests` dependency.
- CTest passed with `2/2` tests:
  `nerdss_unit_tests` and `nerdss_diagnostics_tests`.
- Serial `bin/nerdss` build passed.
- Unified validation passed smoke, unit configure/build/CTest, and regression;
  benchmarks were skipped for this non-computation diagnostics slice.

Artifacts:
- `/private/tmp/nerdss-diagnostics-test-registration-validation/validation_report.json`

Notes:
- Diagnostics unit coverage no longer requires the standalone `g++` command;
  it is registered in the normal CMake/CTest path.
- Remaining traceback gaps are documented in `docs/upgrade/error_model.md`.

## 2026-05-29: Diagnostics Error Boundary

Date: 2026-05-29

Branch: `codex/diagnostics-error-boundaries`

Commit: Pending at validation time

Workstream: Diagnostics and error-message boundaries

Environment:
- OS: macOS/Darwin arm64 local Codex workspace
- Compiler: Apple clang via `g++`
- GSL: Homebrew GSL from existing project configuration
- CMake: Available on `PATH`
- Make: Available on `PATH`
- MPI: Not exercised by this serial diagnostics slice

Commands:
```sh
git diff --check
cmake -S . -B build-upgrade-validation
g++ -std=c++11 -Iinclude tests/unit/test_diagnostics.cpp -o /tmp/nerdss_diagnostics_tests
/tmp/nerdss_diagnostics_tests
cmake --build build-upgrade-validation --target nerdss_unit_tests
ctest --test-dir build-upgrade-validation --output-on-failure
make serial
bin/nerdss --seed nope
tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-diagnostics-error-boundaries-validation
```

Results:
- `git diff --check` passed.
- CMake configure passed.
- Focused diagnostics test compiled and passed as a standalone unit executable.
- Existing CMake unit target built successfully.
- CTest passed for `nerdss_unit_tests`.
- Serial `bin/nerdss` build passed.
- Intentional bad CLI invocation exited with status `2` and printed a
  structured input diagnostic with one parser-boundary trace frame.
- Unified validation passed smoke, unit configure/build/CTest, and regression;
  benchmarks were skipped for this non-computation diagnostics slice.

Artifacts:
- `/tmp/nerdss-diagnostics-error-boundaries-validation/validation_report.json`

Notes:
- Remaining traceback gaps are parser/setup/file I/O call sites that still use
  direct `exit(...)`, `error(...)`, or ad hoc `std::cerr` messages.
- MPI-aware diagnostics remain legacy rank-text errors.
