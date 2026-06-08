# Stack 31 Implicit Lipid Probability Service - 2026-06-08

## Scope

This probability-engine slice moves pure implicit-lipid and compartment
probability kernels behind a dedicated backend-neutral service while preserving
legacy entry points.

- Adds `ImplicitLipidProbabilityService` for 2D/3D implicit-lipid
  dissociation, 2D/3D binding, 2D integral kernels/integration/block distance,
  compartment entry/exit probability, and compartment setup.
- Keeps `ProbabilityEngine` public methods as forwarding wrappers so existing
  callers and legacy reaction wrappers do not change.
- Extends unit coverage to compare the service, facade, and legacy wrappers.

## Local Validation

Commands run on `codex/implicit-lipid-probability-service-stack31-slice`:

- `git diff --check HEAD`: passed.
- `tools/run_static_analysis.sh include/core/probability_engine.hpp include/core/probability/implicit_lipid_probability_service.hpp tests/unit/test_vector_coord.cpp`:
  passed with existing unit-test redundant declaration/readability warnings.
- `cmake -S . -B build-implicit-lipid-probability-service`: passed.
- `cmake --build build-implicit-lipid-probability-service --target nerdss_unit_tests --parallel 4`:
  passed.
- `ctest --test-dir build-implicit-lipid-probability-service --output-on-failure`:
  passed, 3/3 tests.
- `make serial -j4`: passed and rebuilt `bin/nerdss`.
- `python3 -B tools/run_upgrade_validation.py --skip-build --binary bin/nerdss --output-dir /tmp/nerdss-implicit-lipid-probability-service-validation`:
  passed smoke, unit configure/build/ctest, and regression; benchmarks skipped.

## Follow-Up Notes

- Stochastic event selection and topology mutation remain in the legacy
  reaction wrappers.
- Raw `gsl_matrix*` ownership for 2D probability tables remains unchanged; the
  next ProbabilityEngine slice should focus on table ownership/cache interfaces.
