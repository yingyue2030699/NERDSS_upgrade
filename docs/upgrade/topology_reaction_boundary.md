# Topology Reaction Boundary

This document maps the mutable topology operations inside reaction execution so
future slices can separate them from probability and geometry calculations
without changing behavior. The first code slice only names existing slot
bookkeeping used by dissociation; it does not change RNG use, probability
evaluation, molecule iteration, interface-state mutation, or complex split
order.

## Current Mutation Map

| Area | Current entry points | Mutable state |
| --- | --- | --- |
| Dissociation split | `break_interaction`, `determine_parent_complex_IL`, `determine_parent_complex` | `Molecule::bndpartner`, `Molecule::bndlist`, interface binding state, `Molecule::myComIndex`, `Complex::memberList`, `Complex::emptyComList`, `Complex::numberOfComplexes`, `Complex::maxID` |
| Implicit-lipid release | `break_interaction_implicitlipid` | Bound interface state, free/bound lists, monomer destroy lists |
| Creation | `create_molecule_and_complex_from_rxn`, `create_molecule_and_complex_from_transmission_rxn` | Empty molecule slots, empty complex slots, subcell membership, monomer lists, complex counters |
| Destruction and cleanup | `check_for_unimolecular_reactions`, `check_dissociation`, `remove_empty_slots` | Empty molecule/complex lists, member-list indices, binding-partner indices, interface partner indices |

## First Boundary Slice

`include/reactions/topology/topology_operations.hpp` introduces small
behavior-preserving helpers:

- `ReserveDissociationComplexSlot(...)` reserves the complex slot that
  `break_interaction` may fill if dissociation splits the parent complex.
- `ReleaseReservedComplexSlot(...)` undoes that reservation when loop
  correction cancels dissociation or when the dissociated molecules remain in a
  closed loop.
- `RestoreDissociationReactantsToParentComplex(...)` restores both dissociating
  molecules to the original parent complex when loop detection cancels the
  split.
- `ApplyDissociationParentComplexReassignment(...)` applies the already
  computed split member lists, preserves the legacy sort order, updates the new
  complex index, and rewrites member `myComIndex` values.
- `ApplyComplexSlotCompactionMove(...)` copies a live complex into an earlier
  empty slot and rewrites the moved members' `myComIndex` values. It mirrors the
  complex-side move performed during empty-slot compaction without changing the
  main-loop compaction call site yet.

The helpers intentionally preserve the legacy behavior:

- use the most recent `Complex::emptyComList` entry only when it still points to
  an empty complex;
- append a new empty `Complex` otherwise;
- return an appended slot with `pop_back()`;
- return a reused slot by pushing the index back into `Complex::emptyComList`.
- keep the parent-complex split decision in `determine_parent_complex(...)`;
- preserve the legacy sorted member lists produced by non-implicit-lipid
  dissociation reassignment.
- preserve the moved complex member order during empty-slot compaction, while
  updating only the moved members' complex index.

## Behavior Tests

The standalone topology harness now covers:

- empty-slot reservation through append, valid reuse, stale entry retention,
  reused-slot rollback, and repeated tail-reservation release;
- restoring dissociating reactants to their parent complex after loop
  cancellation;
- small and larger dissociation member reassignment, including sorted split
  member lists and untouched non-member molecules;
- reassignment into a reused empty complex slot without growing
  `complexList`;
- the complex-side empty-slot compaction move that rewrites the moved complex
  index and moved members' `myComIndex` values.

Validation for this slice:

| Check | Result | Notes |
| --- | --- | --- |
| Standalone topology helper tests | Passed | `tests/topology/run_topology_tests.sh` covers append reservation, valid empty-slot reuse, stale empty-list entries, and release rollback behavior. |
| CTest unit suite | Passed | Existing unit executable rebuilt and passed. |
| Serial build | Passed | `make serial` rebuilt `bin/nerdss`. |
| Unified validation runner | Passed | Smoke, unit configure/build/CTest, and regression passed. |
| Benchmark mode | Passed | `small_homotrimer` wall time `9.384s`, CPU time `8.982s`. |

## Follow-Up Slices

1. Extend the standalone topology harness beyond slot reservation to cover
   parent-complex reassignment and empty-slot compaction without constructing a
   full simulation.
2. Move `determine_parent_complex_IL` and `determine_parent_complex` behind a
   `TopologyEditor`-style interface after adding tests for loop and split
   cases.
3. Extract binding-interface release helpers shared by normal and
   implicit-lipid dissociation while preserving the current log-write order.
4. Extract creation slot allocation separately from coordinate sampling so
   creation can be tested without changing overlap/resampling RNG order.
5. Extract `remove_empty_slots` into reusable topology compaction operations
   only after validating a destruction case that observes molecule and complex
   index remapping.
