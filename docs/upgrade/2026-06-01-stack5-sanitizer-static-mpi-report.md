# Stack 5 Sanitizer, Static Analysis, And MPI Report

Date: 2026-06-01

Branch: `codex/stack5-sanitizer-static-mpi-report`

Base ref: `personal/codex/validation-integration-stack-5`

Base commit: `092d64314d591fa94518efb0165e7f0b671fa74a` (`Merge stack 4 CI workflow status into stack 5`)

Scope: rerun the existing quality gates that had not been rerun on stack 5: sanitizer build helper, static-analysis helper, and locally available MPI build/smoke checks. No source changes were made.

## Tool Availability

| Tool | Command | Result |
| --- | --- | --- |
| CMake | `cmake --version` | Available, `cmake version 4.3.3`. |
| GSL | `gsl-config --version` | Available, `2.8`. |
| Make | `make --version` | Available, GNU Make 3.81. |
| Compiler | detected by CMake | AppleClang `16.0.0.16000026`. |
| clang-tidy | `clang-tidy --version` | Not available on `PATH`; shell exit 127. |
| MPI compiler wrapper | `mpicxx --version` | Wrapper exists at `/opt/anaconda3/bin/mpicxx`, but exits 127 because `x86_64-apple-darwin13.4.0-clang++` is missing. |
| MPI launcher | `mpirun --version` | Available, MPICH/HYDRA version 3.3.2. |

## Commands And Results

### Sanitizer Builds

Command:

```sh
NERDSS_BUILD_JOBS=4 tools/run_sanitizer_builds.sh
```

Result: PASS, exit 0.

The helper configured and built all default sanitizer modes under `build/sanitizers/`:

- `address`: configured with `-DNERDSS_ENABLE_ASAN=ON`, built target `nerdss` to 100%.
- `undefined`: configured with `-DNERDSS_ENABLE_UBSAN=ON`, built target `nerdss` to 100%.
- `combined`: configured with both sanitizer options enabled, built target `nerdss` to 100%.

Notes:

- CMake emitted a deprecation warning for `cmake_minimum_required(VERSION 3.5)`, but this did not block configuration or build.
- Generated build outputs are ignored by `.gitignore` under `build/`.

### Static Analysis

Command:

```sh
tools/run_static_analysis.sh
```

Result: BLOCKED, exit 127.

Output:

```text
clang-tidy was not found on PATH.
Install clang-tidy or add it to PATH, then rerun this script.
```

Blocker: local `clang-tidy` is not installed or not discoverable on `PATH`, so the script exits before configuring the static-analysis compile database.

### MPI Build

Command:

```sh
make mpi
```

Result: FAIL, exit 2.

The Makefile selected `mpicxx` and attempted the first MPI object compile:

```text
mpicxx -O3  -std=c++0x -I/opt/homebrew/Cellar/gsl/2.8/include -Iinclude  -c src/boundary_conditions/check_if_spans.cpp -o obj/release/boundary_conditions/check_if_spans.o  -Dmpi_
/opt/anaconda3/bin/mpicxx: line 299: x86_64-apple-darwin13.4.0-clang++: command not found
make: *** [obj/release/boundary_conditions/check_if_spans.o] Error 127
```

Blocker: the local MPI compiler wrapper resolves to a missing compiler executable. The NERDSS MPI executable is not produced, so no NERDSS MPI runtime smoke can be attempted from this build.

### MPI Launcher Smoke

Command:

```sh
mpirun -n 2 /bin/echo mpi-launcher-smoke
```

Result in sandbox: FAIL, exit 255.

Sandboxed output:

```text
HYDU_sock_listen ... failed to bind to any port
HYDU_sock_create_and_listen_portstr ... unable to listen on port
HYD_pmci_launch_procs ... unable to create PMI port
```

The same command was rerun outside the sandbox after approval because the failure was a PMI port-bind error.

Result outside sandbox: PASS, exit 0.

Output:

```text
mpi-launcher-smoke
mpi-launcher-smoke
```

Interpretation: the MPI launcher can start two local processes outside the sandbox, but this does not validate NERDSS MPI because `make mpi` is blocked by the broken MPI compiler wrapper.

## Summary

| Gate | Status | Details |
| --- | --- | --- |
| Sanitizer build helper | PASS | `address`, `undefined`, and `combined` CMake sanitizer builds completed. |
| Static-analysis helper | BLOCKED | `clang-tidy` missing from `PATH`; script exited 127. |
| MPI build | FAIL | `mpicxx` points to missing `x86_64-apple-darwin13.4.0-clang++`; `make mpi` exited 2. |
| MPI launcher smoke | PASS outside sandbox | `mpirun -n 2 /bin/echo mpi-launcher-smoke` passed only after rerun outside sandbox. |
| NERDSS MPI runtime smoke | NOT RUN | No MPI NERDSS binary was produced. |

## Remaining Gaps

- Install or expose `clang-tidy` on `PATH`, then rerun `tools/run_static_analysis.sh`.
- Repair the MPI compiler wrapper or install a working MPI toolchain for the host architecture, then rerun `make mpi`.
- After `make mpi` succeeds, add or run a NERDSS-specific MPI smoke using the produced `bin/nerdss_mpi`.
