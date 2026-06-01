#!/usr/bin/env bash
set -euo pipefail

clang_tidy_bin="${CLANG_TIDY_BIN:-}"
if [ -z "$clang_tidy_bin" ]; then
  if command -v clang-tidy >/dev/null 2>&1; then
    clang_tidy_bin="$(command -v clang-tidy)"
  elif [ -x /opt/homebrew/opt/llvm/bin/clang-tidy ]; then
    clang_tidy_bin=/opt/homebrew/opt/llvm/bin/clang-tidy
  elif [ -x /usr/local/opt/llvm/bin/clang-tidy ]; then
    clang_tidy_bin=/usr/local/opt/llvm/bin/clang-tidy
  fi
fi

if [ -z "$clang_tidy_bin" ] || [ ! -x "$clang_tidy_bin" ]; then
  echo "clang-tidy was not found on PATH." >&2
  echo "Install clang-tidy, add it to PATH, or set CLANG_TIDY_BIN." >&2
  exit 127
fi

build_dir="${NERDSS_STATIC_ANALYSIS_BUILD_DIR:-build/static-analysis}"
default_checks="clang-analyzer-*,bugprone-*,performance-*,portability-*"

if [ "$#" -eq 0 ]; then
  set -- src/math src/parser EXEs
fi

cmake \
  -S . \
  -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

tmp_sources="$(mktemp "${TMPDIR:-/tmp}/nerdss-clang-tidy.XXXXXX")"
trap 'rm -f "$tmp_sources"' EXIT

for path in "$@"; do
  if [ -d "$path" ]; then
    find "$path" -type f -name '*.cpp'
  elif [ -f "$path" ]; then
    case "$path" in
      *.cpp) printf '%s\n' "$path" ;;
    esac
  else
    echo "Skipping missing path: $path" >&2
  fi
done | sort -u > "$tmp_sources"

if [ ! -s "$tmp_sources" ]; then
  echo "No C++ source files found for clang-tidy." >&2
  exit 2
fi

tidy_args=(-p "$build_dir")
if [ -n "${NERDSS_CLANG_TIDY_CHECKS:-}" ]; then
  tidy_args+=("-checks=${NERDSS_CLANG_TIDY_CHECKS}")
elif [ ! -f .clang-tidy ]; then
  tidy_args+=("-checks=$default_checks")
fi

if [ "$(uname -s)" = "Darwin" ] && command -v xcrun >/dev/null 2>&1; then
  sdk_path="$(xcrun --show-sdk-path 2>/dev/null || true)"
  if [ -n "$sdk_path" ]; then
    tidy_args+=("--extra-arg=-isysroot" "--extra-arg=$sdk_path")
    if [ -d "$sdk_path/usr/include/c++/v1" ]; then
      tidy_args+=("--extra-arg=-isystem" "--extra-arg=$sdk_path/usr/include/c++/v1")
    fi
  fi
fi

while IFS= read -r source_file; do
  echo "$clang_tidy_bin $source_file"
  "$clang_tidy_bin" "${tidy_args[@]}" "$source_file"
done < "$tmp_sources"
