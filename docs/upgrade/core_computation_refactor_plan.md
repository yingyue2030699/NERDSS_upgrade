# Core Computation Refactor Plan

This plan starts the behavior-preserving refactor of NERDSS core computation.
The goal is to make the scientific engine easier to test, optimize, and later
map to GPU-oriented backends without changing reaction probabilities, RNG order,
trajectory math, restart semantics, or legacy input/output compatibility.

## Principles

- Preserve validated behavior before moving code. Every core slice must pass
  `tools/run_upgrade_validation.py`.
- Refactor by cohesive modules, not by a mechanical one-header/one-source-file
  pattern. Headers should expose stable module boundaries; implementation files
  may group related algorithms when that improves locality and reviewability.
- Separate orchestration from math. Executables and simulation loops should call
  domain services; pure numeric kernels should avoid file I/O, logging, global
  state, and container mutation unless that is their explicit responsibility.
- Keep diagnostics cheap in hot paths. Structured errors and tracebacks belong
  at parser, setup, I/O, and orchestration boundaries first. Inner loops should
  use compile-time-disabled or branch-minimal tracing hooks.
- Preserve CPU scalar behavior as the reference backend. Any GPU backend must
  prove parity against CPU scalar outputs before it becomes selectable.

## Target Module Boundaries

| Boundary | Responsibility | First extraction target |
| --- | --- | --- |
| `nerdss::core` | Shared core vocabulary and behavior-preserving facades over legacy primitives. | Math engine facade and validation hooks. |
| Math engine | Pure geometry, vector, matrix, probability-table, and special-function kernels. | Header-only CPU scalar facade over existing `Coord`, `Vector`, and matrix functions. |
| Reaction engine | Candidate discovery, probability evaluation, and reaction execution split into separate services. | Probability calculations before topology mutation. |
| Trajectory engine | Propagation vectors, rotations, reflection, overlap/resampling, and boundary dispatch. | Boundary dispatch wrappers now forward through `nerdss::core::TrajectoryEngine` with unchanged call order. |
| Topology editor | Molecule/complex binding, splitting, creation/destruction, freelists, and counters. | Destroyed-slot and complex compaction helpers after validation guards. |
| Diagnostics | Structured errors, trace context, and low-cost logging surfaces. | Parser/setup/I/O errors first; hot-loop tracing only under compile-time flags. |

## Math Engine Roadmap

1. Add a `nerdss::core::MathEngine` CPU scalar facade that groups numeric
   primitives under a single header. Current slices cover vector/matrix helpers
   plus matrix-vector rotation, rounded conservation predicates, spherical
   coordinate, geodesic-distance, angular wrapping, association-angle predicate,
   spherical association-position, signed projected-angle, angular-displacement,
   arbitrary orthogonal-vector, sign-flip orientation, spherical inner-coordinate
   frame/translation, spherical frame-axis rotation, factorial, Numerical
   Recipes log-gamma, gamma-factorial, and association rotation-angle partition
   kernels. The distance-kernel slice adds shared Euclidean, squared Euclidean,
   planar XY, sphere-surface, and z-plane distance helpers; legacy interface
   separation and implicit-lipid surface-distance callers now forward through
   these pure kernels without changing thresholds or cross-list mutation.
2. Add unit coverage proving the facade delegates to legacy `Coord`, `Vector`,
   and matrix behavior.
3. Move pure probability functions behind explicit service names while keeping
   legacy functions as forwarding wrappers. The first slices move the 1D/3D
   association, rebinding-ratio, 2D table-integrand, 2D table interpolation,
   2D PIR table interpolation, 2D table rebinding-ratio, 2D table sizing, and
   implicit-lipid probability kernels behind `nerdss::core::ProbabilityEngine`.
   Current slices additionally cover the 2D implicit-lipid binding
   probability integration and block-distance solve while preserving legacy
   wrapper mutation of `R2D`, plus the same-complex loop-closure association
   probability used by reaction probability evaluation. The 2026-06-01
   integrator slice also moves the semi-infinite GSL integration retry/fallback
   algorithm used by 2D reaction-table generation behind `ProbabilityEngine`
   while keeping the legacy `integrator` function as a forwarding wrapper.
   The 2D diffusion-table binning helper now also lives behind
   `ProbabilityEngine` so bimolecular and implicit-lipid reaction paths share
   one table quantization rule. Reaction search-radius/RMax arithmetic for
   1D, 2D, and 3D candidate checks is also exposed through pure
   `ProbabilityEngine` helpers while legacy reaction paths keep forwarding
   through the same formulas. The rotational diffusion contribution used by
   2D/3D bimolecular and implicit-lipid probability calculations is now a pure
   `ProbabilityEngine` helper as well, preserving the legacy cosine
   displacement relation and `2 * dimension * dt` denominator. The next
   probability slice moves bimolecular and surface association-rate conversion
   kernels into `ProbabilityEngine`: 1D area scaling, 2D length scaling,
   3D surface/symmetric multipliers, and 3D-to-surface rate doubling now have
   named pure helpers while the legacy reaction callers retain all mutation,
   table allocation, and warning behavior. The complex displacement guard now
   also forwards the pure Einstein displacement and scaled-threshold arithmetic
   through `ProbabilityEngine`, keeping coordinate mutation and cancellation
   decisions in the legacy association path. The Poisson event-probability
   helper now centralizes `1 - exp(-lambda)` for unimolecular, dissociation,
   state-change, and loop-closure probability calculations while preserving
   RNG draws, counters, topology mutation, warnings, and cancellation in the
   legacy reaction callers. The closed-loop dissociation correction ratio now
   also lives behind `ProbabilityEngine`, reusing the same Poisson kernel for
   `(1 - exp(-lambda)) / lambda` while leaving RNG cancellation, rebinding-rate
   selection warnings, and topology mutation in the legacy dissociation path.
   The 3D bimolecular association parameter bundle now exposes the pure
   diffusion-limited rate, intrinsic rate, alpha, and association-probability
   coefficient behind `ProbabilityEngine` while preserving pair search,
   reweighting, RNG, and molecule probability-vector mutation in the legacy
   3D reaction path.
4. Separate mutable topology operations from probability calculations.
5. Introduce backend-neutral data-shape documentation for future GPU work:
   structure-of-arrays candidates, batchable kernels, and RNG constraints.

## Trajectory Engine Roadmap

1. Start with behavior-preserving boundary dispatch. The first slice moves the
   `sweep_separation_complex_rot`, `sweep_separation_complex_rot_memtest`, and
   `sweep_separation_complex_rot_memtest_cluster` sphere/box selection behind
   `nerdss::core::TrajectoryEngine`; the legacy entry points remain forwarding
   wrappers.
2. Keep overlap loops, resampling, reflection calls, and RNG draws in the
   existing implementations until each path has targeted regression coverage.
3. Next candidates are small trajectory-coordinate transforms whose inputs and
   outputs can be compared directly before extracting mutation-heavy reflection
   or complex-resampling logic.

## Error Handling And Tracebacks

The structured error model is already documented. The next implementation
layers should proceed in this order:

1. Parser and CLI errors: no hot-loop cost.
2. File I/O and restart errors: include path and operation.
   The serial restart file-open path now uses structured file I/O diagnostics
   and is covered by the smoke-runner CLI checks.
3. Setup invariants: include molecule/template/reaction identifiers.
4. Core-loop diagnostics: compile-time gated trace scopes with zero work when
   disabled.
5. MPI diagnostics: rank-aware context after serial behavior is stable.

Tracebacks should be explicit call-context stacks rather than C++ exception
unwinding in hot numeric paths. The first production target is actionable error
messages, not broad exception conversion.

## Validation Rules

Before each behavior-sensitive core refactor:

```sh
python3 tools/run_upgrade_validation.py --skip-build --binary bin/nerdss
```

For changes touching probability, trajectory, or topology code, also run at
least one benchmark case:

```sh
python3 tools/run_upgrade_validation.py \
  --skip-build \
  --binary bin/nerdss \
  --benchmarks \
  --benchmark-case small_homotrimer
```

Any observed nondeterminism must be fixed or documented before extraction
continues.
