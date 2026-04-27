#!/bin/bash
# Build libinfones.a from depends/InfoNES/src and stage it under this
# widget's libs/ directory. After this script has been run once and the
# resulting libinfones.a + libs/InfoNES/*.h are committed, the widget
# builds without needing depends/ at all.
#
# Defaults: host x86_64 build with system g++/ar.
# Cross-compile: set CXX / AR before invoking, e.g.
#   CXX=arm-linux-gnueabihf-g++ AR=arm-linux-gnueabihf-ar ./build_libinfones.sh
#
# The InfoNES_HSync cooperative-stop hook (see src/infones_glue/) requires
# the core to be built with -DINFONES_AWTK_GLUE=1.

set -euo pipefail

LIBS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$LIBS_DIR/../../.." && pwd)"
INFONES_SRC="$REPO_ROOT/depends/InfoNES/src"
STAGE_HDR="$LIBS_DIR/InfoNES"

if [ ! -d "$INFONES_SRC" ]; then
  echo "error: $INFONES_SRC not found — depends/InfoNES is required to (re)build the prebuilt lib" >&2
  exit 1
fi

CXX=${CXX:-g++}
AR=${AR:-ar}
CXXFLAGS_COMMON="-Wall -Wno-write-strings -fPIC -DINFONES_AWTK_GLUE=1 -I$INFONES_SRC"

OBJ_DIR="$(mktemp -d)"
trap 'rm -rf "$OBJ_DIR"' EXIT

echo ">> building libinfones.a (host: $($CXX -dumpmachine))"

# pAPU is sensitive to optimisation in some gcc versions — keep it at -O0.
$CXX $CXXFLAGS_COMMON -O0 -c "$INFONES_SRC/InfoNES_pAPU.cpp" -o "$OBJ_DIR/InfoNES_pAPU.o"

# Rest at -O2.
for f in InfoNES.cpp InfoNES_Mapper.cpp K6502.cpp; do
  $CXX $CXXFLAGS_COMMON -O2 -c "$INFONES_SRC/$f" -o "$OBJ_DIR/${f%.cpp}.o"
done

$AR rcs "$LIBS_DIR/libinfones.a" \
  "$OBJ_DIR/InfoNES.o" \
  "$OBJ_DIR/InfoNES_Mapper.o" \
  "$OBJ_DIR/InfoNES_pAPU.o" \
  "$OBJ_DIR/K6502.o"

echo ">> staging headers into $STAGE_HDR"
mkdir -p "$STAGE_HDR"
# Public headers needed at compile time by the glue + internal headers
# pulled in by them (Types, K6502_rw, Mapper).
cp "$INFONES_SRC"/*.h "$STAGE_HDR/"

echo ">> done"
ls -la "$LIBS_DIR/libinfones.a" "$STAGE_HDR/" | head -20
