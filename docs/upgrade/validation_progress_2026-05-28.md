# Validation Progress - 2026-05-28

This note records the validation, benchmark, coverage, and agent status from
the local integration workflow on `codex/validation-integration`.

## Integration Context

Local branch:

```text
codex/validation-integration
```

Integrated slices at the time of this run:

- Smoke runner and serial build fixes.
- Deterministic regression harness.
- Stochastic seed-set validation harness.
- Minimal CTest unit test target.
- Create/destroy crash hardening.
- Legacy restart crash hardening.
- Benchmark harness.
- Profiling command guide.

The local worktree was dirty only because of generated build directories:

```text
?? build-coverage/
?? build-unit-validation/
```

## Validation Results

| Check | Result | Notes |
| --- | --- | --- |
| Serial smoke runner | Passed | Built serial NERDSS and completed the generated smoke simulation. |
| CTest unit target | Passed | `1/1` tests passed. |
| Regression manifest | Passed | All three selected validation cases passed. |
| Stochastic seed set | Passed | `create_destroy_stochastic_seed_set` passed seeds `31001` through `31005`. |
| Restart regression | Passed | `homo_trimer_restart_from_1000` passed with legacy restart input. |

The restart regression depends on the legacy `RNGwrite` parser hardening. The
create/destroy seed-set regression depends on skipping destroyed molecules in
the overlap-check loop.

## Coverage Snapshot

Coverage was collected after smoke and the full validation manifest using the
Xcode `llvm-cov` toolchain.

| Metric | Coverage |
| --- | ---: |
| Lines | 17.43% |
| Functions | 20.25% |
| Branches | 16.32% |

This is an early integration coverage snapshot, not a target threshold. It is
useful for trend tracking and for identifying large untested regions before
deeper refactoring.

## Benchmark Snapshot

All benchmark manifest cases completed with exit status `0` using seed `12345`.

| Case | Wall time (s) | CPU time (s) | Output files | Output bytes |
| --- | ---: | ---: | ---: | ---: |
| `small_homotrimer` | 8.668 | 8.560 | 24 | 8,828,354 |
| `medium_michaelis_menten` | 1.036 | 0.871 | 15 | 164,072 |
| `representative_implicit_lipid` | 2.068 | 1.979 | 24 | 1,510,702 |
| `large_clathrin_short` | 0.419 | 0.256 | 15 | 442,377 |

Benchmark artifacts were written outside the repository:

```text
/tmp/nerdss-benchmark-baseline/
/tmp/nerdss-benchmark-representative/
/tmp/nerdss-benchmark-large/
```

## Profiling Status

`tools/profile_commands.sh` generated the expected macOS `sample` command
block for the homotrimer case. A live attempt to attach `sample` to the running
NERDSS process was blocked by macOS permissions:

```text
sample cannot examine process ...; try running with `sudo`.
```

The same homotrimer case was rerun successfully while trying to collect timing
data with `/usr/bin/time -lp`, but the sandbox blocked the kernel clock query
after the simulation completed. The benchmark runner timings above remain the
current reliable baseline. A full hotspot report still needs an elevated
macOS Instruments or `sample` run, or a Linux `perf`/`gprof` profiling run.

## Agent Status

| Agent slice | Branch | Status |
| --- | --- | --- |
| Main loop first slice | `codex/upgrade-main-loop-first-slice` | Docs-only slice committed and pushed to `personal`. |
| Run manifest writer | `codex/upgrade-run-manifest-writer` | Implemented, validated, committed, and pushed to `personal`. |
| CI regression workflow | `codex/upgrade-ci-regression` | Implemented and validated locally; push blocked because the OAuth credential lacks GitHub `workflow` scope. |
| Expanded validation | `codex/upgrade-expanded-validation` | Implemented, validated, committed, and pushed to `personal`. |

## Integration Update

The local integration branch was advanced after the initial note by merging the
completed non-core slices for baseline policy, architecture mapping, style
tooling, sanitizer/static-analysis tooling, input schema conversion, run
manifest writing, main-loop extraction planning, and expanded regression
validation.

A small CLI error-handling slice was also added locally:

- `parse_command` now reports missing values for flags such as `-f` and `-s`
  instead of indexing past `argv`.
- `--help` prints usage and exits successfully.
- Running without `-f` or `-r` now exits with the structured input error code.
- The smoke runner now includes negative CLI checks for `--help`, missing `-f`
  value, and missing `-s` value.

Post-merge validation:

| Check | Result | Notes |
| --- | --- | --- |
| Python syntax checks | Passed | Smoke, regression, benchmark, input converter, and manifest inspector scripts compiled with `py_compile`. |
| JSON schema syntax | Passed | Input and run-manifest schemas loaded with `python3 -m json.tool`. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Smoke runner | Passed | `--skip-build` smoke passed and CLI checks passed. |
| Expanded regression suite | Passed | `PASS: 7 regression case(s)`. |
| Upgrade validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed; benchmark mode also passed with `medium_michaelis_menten`. |

## Validation Runner Update

Added `tools/run_upgrade_validation.py` as a top-level orchestrator for the
upgrade workflow. It runs the existing smoke runner, CMake unit tests, CTest,
the regression harness, and optional benchmark cases, then writes a single
`validation_report.json` plus per-step stdout/stderr logs.

This runner is intended as the standard local command before behavior-sensitive
refactors and as the future CI entry point once workflow pushes are available.

## Workflow Follow-Up

The first unified validation run exposed nondeterministic `DATA/restart.dat`
output in `implicit_lipid_fresh_small`: the serialized `membrane` line could
write an uninitialized `Membrane::No_protein` value. The `Membrane` primitive
fields are now default-initialized so fresh fixed-seed runs produce stable
restart metadata.

The regression harness also now creates the parent directory passed through
`--tmp-root`, and the unified validation runner asks the regression harness to
write `regression_report.json` alongside the top-level
`validation_report.json`.

## Core Refactor Start

Started the core-computation refactor track with
`docs/upgrade/core_computation_refactor_plan.md` and the first
`nerdss::core::MathEngine` CPU scalar facade. This is a boundary-only slice:
existing `Coord`, `Vector`, and matrix algorithms remain the implementation of
record, while the new facade gives future probability, trajectory, and
GPU-oriented work a clear place to attach.

Validation for the first slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added coverage for the `MathEngine` facade. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and 7-case regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.738s`, CPU time `8.559s`. |

## Diagnostics Boundary Start

Added the first low-cost diagnostics boundary in `include/core/diagnostics.hpp`.
It provides fixed-capacity trace stacks, scoped frames, and structured
diagnostics that map to the existing error category and exit-code model. The
header does not allocate or format strings unless a diagnostic is explicitly
formatted, and trace scopes remain compile-time gated for future hot-loop use.

## Probability Engine Start

Started moving pure probability kernels behind core service interfaces by adding
`include/core/probability_engine.hpp`. The 3D and 1D association probability
formulas now live behind `nerdss::core::ProbabilityEngine`, while the legacy
`passocF` and `passocF_1D` functions remain as forwarding wrappers so existing
reaction call sites and validation baselines stay unchanged.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for 1D and 3D association probability. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and 7-case regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.655s`, CPU time `8.518s`. |

## Rebinding Probability Ratio Facade

Continued the probability-kernel extraction by moving the 3D and 1D rebinding
probability ratio formulas behind `nerdss::core::ProbabilityEngine`. The legacy
`pirr_pfree_ratio_psF` and `pirr_pfree_ratio_psF_1D` functions now forward to
the core service so existing reaction call sites remain stable while future
math-engine and GPU-oriented work gets a clearer pure-kernel boundary.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for 1D and 3D rebinding probability ratios. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss` with the forwarding wrappers. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.584s`, CPU time `8.404s`. |

## 2D Table Integrand Facade

Moved the 2D survival and irreversible-probability table integrand formulas
behind `nerdss::core::ProbabilityEngine`. The legacy `survival_function` and
`pir_function` GSL callback signatures remain in place as adapters from
`IntegrandParams` to explicit scalar arguments. This keeps matrix construction
compatible while making the formulas easier to unit test and batch in future
math-engine work.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-callback checks for finite-rate and absorbing 2D integrands. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.643s`, CPU time `8.557s`. |

Follow-up: moved the 2D free-diffusion normalization integrand behind the same
service facade as `FreeDiffusionNormIntegrand2D`, with `norm_function` retained
as the GSL callback adapter.

Validation for the follow-up:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-callback coverage for the 2D norm integrand. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.602s`, CPU time `8.441s`. |

## 2D Free-Diffusion Probability Facade

Extracted the free-diffusion probability density used by
`DDpirr_pfree_ratio_ps` into
`nerdss::core::ProbabilityEngine::FreeDiffusionProbability2D`. The remaining
legacy function still owns the GSL matrix lookups and ratio orchestration, which
keeps this slice limited to the pure scalar math.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added a radial-normalization relation check against `FreeDiffusionNormIntegrand2D`. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.395s`, CPU time `8.371s`. |

## Implicit-Lipid Dissociation Facade

Started the implicit-lipid probability extraction by moving the closed-form 2D
and 3D dissociation probabilities behind
`nerdss::core::ProbabilityEngine`. The legacy `dissociate2D` and
`dissociate3D` functions remain as wrappers, preserving existing reaction call
sites and units while making the scalar formulas directly testable.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for 2D and 3D implicit-lipid dissociation. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.687s`, CPU time `8.529s`. |

## Implicit-Lipid 3D Binding Facade

Moved the closed-form 3D implicit-lipid binding probability behind
`nerdss::core::ProbabilityEngine::ImplicitLipidBindingProbability3D`. The
legacy `pimplicitlipid_3D` wrapper remains in place and now adapts `paramsIL`
to scalar service arguments.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for separated and contact-distance branches. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.684s`, CPU time `8.583s`. |

## Compartment Probability Facade

Moved the compartment entry and exit binding probabilities behind
`nerdss::core::ProbabilityEngine` as `CompartmentEntryProbability` and
`CompartmentExitProbability`. The legacy `prob_entering_compartment` and
`prob_exiting_compartment` functions remain as `paramsIL` adapters.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper checks for entry and exit probabilities. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.656s`, CPU time `8.512s`. |

## Implicit-Lipid 2D Integral Kernel Facade

Moved the scalar kernel used by the implicit-lipid 2D integration callback
behind `nerdss::core::ProbabilityEngine::ImplicitLipidIntegralKernel2D`. The
legacy `function2D` callback remains the GSL adapter from `paramsIL`.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-callback coverage for `function2D`. |
| Serial build | Passed | Incremental `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.894s`, CPU time `8.704s`. |

## Spherical Math Kernel Facade

Moved spherical coordinate helpers behind `nerdss::core::MathEngine`: radius,
cartesian/spherical conversion, theta/phi wrapping, spherical-angle addition,
and binding-radius conversion on a sphere. The legacy free functions in
`functions_for_spherical_system.cpp` remain as forwarding wrappers.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added direct `MathEngine` checks for spherical radius, conversion, wrapping, and binding-radius kernels. |
| Serial build | Passed | Incremental `make serial` rebuilt the legacy spherical wrapper translation unit. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.685s`, CPU time `8.487s`. |

## Association Angle Predicate Facade

Moved association angle predicates behind `nerdss::core::MathEngine`:
near-equality checks, parallel-angle detection, and angle-sign orientation. The
legacy `areSameAngle`, `areParallel`, and `angleSignIsCorrect` functions remain
as forwarding wrappers.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper coverage for all three predicates. |
| Serial build | Passed | Incremental `make serial` rebuilt the predicate wrappers. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.687s`, CPU time `8.590s`. |

## Geodesic Distance Facade

Moved the spherical geodesic-distance formula behind
`nerdss::core::MathEngine::GeodesicDistance`. The legacy
`get_geodesic_distance` function remains as a forwarding wrapper for existing
spherical association call sites.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper coverage for a quarter-circumference case. |
| Serial build | Passed | Incremental `make serial` rebuilt the legacy wrapper. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.782s`, CPU time `8.551s`. |

## Arbitrary Orthogonal Vector Facade

Moved the arbitrary orthogonal-vector helper behind
`nerdss::core::MathEngine::CreateArbitraryOrthogonalVector`. The legacy
`create_arbitrary_vector` function remains as a forwarding wrapper and preserves
its caller-visible input-normalization side effect.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper coverage and verified the legacy wrapper still normalizes the input vector. |
| Serial build | Passed | Incremental `make serial` rebuilt the legacy wrapper and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.074s`, CPU time `7.849s`. |

## Sign-Flip Orientation Facade

Moved the quaternion-backed association orientation predicate behind
`nerdss::core::MathEngine::RequiresSignFlip`. The legacy `requiresSignFlip`
function remains as a forwarding wrapper for association rotation code.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper coverage for the ordinary z-axis projection path and the x-axis fallback path. |
| Serial build | Passed | Incremental `make serial` rebuilt the legacy wrapper and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.060s`, CPU time `7.879s`. |

## Signed Projected-Angle Facade

Moved the shared XY-projected signed-angle calculation used by `calculate_phi`
and `calculate_omega` behind
`nerdss::core::MathEngine::SignedProjectedAngleOnXY`. The reaction-level
wrappers still perform their existing transform, normal selection, and
tolerance choices before delegating the pure angle kernel.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added direct `MathEngine` coverage for sign flip, sign preservation, and endpoint handling. |
| Serial build | Passed | Incremental `make serial` rebuilt `calculate_phi`, `calculate_omega`, and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.073s`, CPU time `7.868s`. |

## Angular Displacement Facade

Moved the signed angular-displacement kernel used by
`calc_one_angular_displacement` behind
`nerdss::core::MathEngine::SignedAngularDisplacement`. The legacy wrapper still
extracts vectors from molecule and complex state, then delegates the pure
vector calculation.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added direct `MathEngine` coverage for signed displacement and the zero-vector guard. |
| Serial build | Passed | Incremental `make serial` rebuilt `calc_one_angular_displacement` and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.064s`, CPU time `7.867s`. |

## 2D Table Interpolation Facade

Moved the legacy `get_prevNorm` and `get_prevSurv` matrix interpolation
helpers behind `nerdss::core::ProbabilityEngine` as table step-size and
previous-probability lookup services. The legacy functions remain forwarding
wrappers for existing 2D reaction-table call sites.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added synthetic GSL matrix coverage comparing facade and legacy lookup wrappers. |
| Serial build | Passed | Incremental `make serial` rebuilt `get_prevNorm`, `get_prevSurv`, and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.474s`, CPU time `8.370s`. |

## 2D PIR Table Interpolation Facade

Moved the legacy `calc_pirr` inverse-distance interpolation kernel behind
`nerdss::core::ProbabilityEngine::IrreversibleProbabilityTable2D`. The legacy
function remains a forwarding wrapper for existing rebinding-ratio table
call sites.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added synthetic GSL matrix coverage for off-diagonal interpolation and diagonal lookup behavior. |
| Serial build | Passed | Incremental `make serial` rebuilt `calc_pirr` and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.675s`, CPU time `8.409s`. |

## 2D Table Rebinding-Ratio Facade

Moved the `DDpirr_pfree_ratio_ps` composition behind
`nerdss::core::ProbabilityEngine::RebindingProbabilityRatioTable2D`. The
legacy function remains a forwarding wrapper and the core service now owns the
step-size, free-probability, norm lookup, PIR lookup, and tolerance logic in one
place.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added synthetic GSL matrix coverage comparing the core service to the legacy wrapper. |
| Serial build | Passed | Incremental `make serial` rebuilt `DDpirr_pfree_ratio_ps` and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.623s`, CPU time `8.381s`. |

## 2D Table Size Facade

Moved the legacy `size_lookup` table-count kernel behind
`nerdss::core::ProbabilityEngine::TableSize2D`. The legacy wrapper still
accepts `Parameters` for existing callers and forwards the timestep into the
core service.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added facade-vs-wrapper coverage for the 2D table count calculation. |
| Serial build | Passed | Incremental `make serial` rebuilt `size_lookup` and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `9.308s`, CPU time `8.745s`. |

## Spherical Association Position Facade

Moved the `find_position_after_association` spherical arc-position formula
behind `nerdss::core::MathEngine::AssociationPositionOnSphere`. The legacy
function remains a forwarding wrapper and keeps its existing NaN error guard.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added direct `MathEngine` coverage proving the new point stays on the sphere and moves by the requested arc length. |
| Serial build | Passed | Incremental `make serial` rebuilt `functions_for_spherical_system` and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `8.871s`, CPU time `8.593s`. |

## Rounded Conservation Predicate Facade

Moved the rounded vector magnitude and angle comparisons used by
`conservedMags` and `conservedRigid` behind `nerdss::core::MathEngine`. The
legacy functions still own the molecule/complex iteration and now delegate only
the pure vector predicates.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| CTest unit suite | Passed | Added direct `MathEngine` coverage for rounded magnitude and angle conservation predicates. |
| Serial build | Passed | Incremental `make serial` rebuilt `conservedMags`, `conservedRigid`, and `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `9.134s`, CPU time `8.735s`. |
