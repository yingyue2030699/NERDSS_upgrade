# MPI Build Troubleshooting

This page records local MPI build checks and the current workaround for stale
MPI compiler wrappers. It is documentation-only and does not change the MPI
source or runtime policy.

## Build Path

The Makefile MPI target builds `bin/nerdss_mpi` from `EXEs/nerdss_mpi.cpp`,
adds `-Dmpi_`, and includes the `debug`, `io_mpi`, and `mpi` source
directories. The MPI path uses the compiler selected by the Makefile variable
`CC`; the default for `make mpi` is `mpicxx`.

Because `CC` is assigned inside the Makefile, pass compiler overrides as make
command-line variables rather than plain environment variables:

```sh
make mpi CC=/path/to/working/mpicxx
```

## MPICH Backend Override

On the current macOS validation machine, `mpicxx` resolves to the Anaconda
MPICH wrapper:

```sh
command -v mpicxx
mpicxx -show
```

The wrapper reports a missing backend compiler named
`x86_64-apple-darwin13.4.0-clang++`, so `make mpi` fails before NERDSS source
files are compiled. The wrapper supports the standard MPICH `MPICH_CXX`
backend override. A local compile/link probe succeeded with:

```sh
MPICH_CXX='clang++ -arch x86_64' mpicxx -show
MPICH_CXX='clang++ -arch x86_64' make mpi
```

The `-arch x86_64` flag is needed on this machine because the Anaconda MPICH
libraries are x86_64-only.

## Runtime Checks

MPI runtime launch can be restricted inside sandboxed environments. Run launcher
checks from a normal terminal when local socket binding is blocked:

```sh
mpirun -n 2 /bin/echo mpi-launcher-smoke
```

After `bin/nerdss_mpi` is built, verify the binary and run a short validation
case:

```sh
file bin/nerdss_mpi
otool -L bin/nerdss_mpi
mpirun -n 2 ./bin/nerdss_mpi \
  -f sample_inputs/VALIDATE_SUITE/homoTrimer/parmTri6.inp \
  -s 123
```

## Follow-Up

Future tooling can make the override more explicit, for example by introducing
an `MPICXX` Makefile variable that feeds the MPI `CC` selection. That change
should be kept separate from source-level MPI diagnostics because this blocker
is caused by the local wrapper backend, not by NERDSS code.
