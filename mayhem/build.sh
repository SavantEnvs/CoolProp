#!/usr/bin/env bash
# mayhem/build.sh — build CoolProp (static lib, sanitized) + the libFuzzer harnesses in
# mayhem/harnesses, their -standalone reproducers, and the known-answer oracle (normal flags).
# CPM dependencies come from the pre-fetched, pinned copies in $COOLPROP_DEPS (Dockerfile), via
# upstream's own COOLPROP_VENDORED_DEPS_DIR offline mechanism, so this runs air-gapped.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${COOLPROP_DEPS:=/opt/toolchains/deps/coolprop-cpm}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS
cd "$SRC"

common_cmake=(-G Ninja -DCOOLPROP_STATIC_LIBRARY=ON -DCOOLPROP_SHARED_LIBRARY=OFF
  -DCOOLPROP_USE_CCACHE=OFF -DCOOLPROP_NO_EXAMPLES=ON -DBUILD_TESTING=OFF
  -DCOOLPROP_VENDORED_DEPS_DIR="$COOLPROP_DEPS" -DCOOLPROP_REQUIRE_VENDORED_DEPS=ON
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON)

# 1) Sanitized + instrumented library (bld-* is gitignored upstream: `/bld*`).
# The only C source in the library is the vendored miniz (externals/miniz-3.1.1/miniz.c), whose
# inflater computes `NULL + 0` for an empty input buffer (UB in C, defined in C++): UBSan's
# pointer-overflow check fires on the empty input before anything else runs, so it is relaxed for
# C sources only. ASan and every other UBSan check stay on and halting; C++ keeps all of them.
: "${C_SAN_RELAX=$( [ -n "$SANITIZER_FLAGS" ] && echo -fno-sanitize=pointer-overflow )}"
# Superancillaries: by default loading the embedded fluid library eagerly builds the superancillary
# curves of all ~120 fluids, about 31 s under ASan before the first input runs, which Mayhem's
# libFuzzer sanity check rejects. LAZY_LOAD_SUPERANCILLARIES (upstream's own define, used for its
# Debug test runner) builds each fluid's curves on first use instead: same code, reached per fluid.
# It changes an inline function in include/CoolProp/CoolPropFluid.h, so the library AND the
# harnesses must all be compiled with it.
LAZY=-DLAZY_LOAD_SUPERANCILLARIES
cmake -S . -B bld-fuzz "${common_cmake[@]}" -DCMAKE_BUILD_TYPE=None \
  -DCMAKE_C_FLAGS="-O1 $SANITIZER_FLAGS $C_SAN_RELAX -fsanitize=fuzzer-no-link $DEBUG_FLAGS" \
  -DCMAKE_CXX_FLAGS="-O1 $LAZY $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
cmake --build bld-fuzz -j"$MAYHEM_JOBS" --target CoolProp

inc=(-I"$SRC/include" -I"$SRC/src" -isystem "$COOLPROP_DEPS/fmt/include" -isystem "$COOLPROP_DEPS/Eigen"
  -isystem "$COOLPROP_DEPS/nlohmann_json/include" -isystem "$COOLPROP_DEPS/msgpack-c/include"
  -isystem "$COOLPROP_DEPS/boost_headers" -isystem "$COOLPROP_DEPS/valijson/include"
  -DVALIJSON_USE_EXCEPTIONS=1)  # as CMakeLists.txt sets it for the library (detail/json.h, UNIFAC harness)
lib_fuzz="$SRC/bld-fuzz/libCoolProp.a"

# 2) Harnesses: libFuzzer binary + standalone reproducer each.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$SRC/bld-fuzz/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$SRC/bld-fuzz/standalone_main.o"
for h in "$SRC"/mayhem/harnesses/fuzz_*.cpp; do
  n="$(basename "$h" .cpp)"
  $CXX -std=gnu++17 -O1 $LAZY $SANITIZER_FLAGS $DEBUG_FLAGS "${inc[@]}" -c "$h" -o "$SRC/bld-fuzz/$n.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$SRC/bld-fuzz/$n.o" "$SRC/bld-fuzz/lsan_off.o" \
    "$lib_fuzz" -ldl -lpthread -lm -o "/mayhem/$n"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$SRC/bld-fuzz/$n.o" "$SRC/bld-fuzz/standalone_main.o" \
    "$SRC/bld-fuzz/lsan_off.o" "$lib_fuzz" -ldl -lpthread -lm -o "/mayhem/$n-standalone"
done

# 3) Functional oracle: the library again with normal flags + the known-answer program.
cmake -S . -B bld-test "${common_cmake[@]}" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build bld-test -j"$MAYHEM_JOBS" --target CoolProp
$CXX -std=gnu++17 -O1 $COVERAGE_FLAGS "${inc[@]}" "$SRC/mayhem/harnesses/oracle_test.cpp" \
  "$SRC/bld-test/libCoolProp.a" -ldl -lpthread -lm -o "$SRC/bld-test/coolprop_oracle"
