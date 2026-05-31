#!/usr/bin/env sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir="${repo_root}/tests/topology/.build"
binary="${build_dir}/test_topology_operations"

mkdir -p "${build_dir}"

: "${CXX:=g++}"

"${CXX}" -std=c++11 -I"${repo_root}/include" \
    "${repo_root}/tests/topology/test_topology_operations.cpp" \
    "${repo_root}/src/classes/class_Coord.cpp" \
    "${repo_root}/src/classes/class_Vector.cpp" \
    -o "${binary}"

"${binary}"
