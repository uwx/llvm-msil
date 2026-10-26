#!/usr/bin/env bash
# Builds SDL3 from source through llvm-msil and runs it.
#
#   SDL3_DIR=/path/to/SDL ./sdl3_port.sh <llvm-msil>
#
# This is a port test, not an SDL userland test: SDL's own C is compiled to
# IR, linked, emitted as CIL and run, so a bug in the backend shows up as a
# wrong answer or a crash rather than as an unresolved symbol. (sdl.sh is the
# other side of that: a program that *calls* SDL through pinvoke.)
#
# The translation units listed below are the ones that compile and emit. The
# rest of SDL is either C++ (GameInput), a platform adapter for a backend this
# test does not build, or uses x86 intrinsics the backend rejects by design.
# sdl3_stubs.c supplies the few entry points the retained code calls into the
# excluded parts.
#
# Configuration:
#   SDL3_DIR  a checked-out SDL3 source tree, configured once (skipped when
#             unset). sdl3-msil.patch is applied here if the tree is clean.
set -eu

MSIL=${1:?usage: sdl3_port.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
# llvm-link and llvm-dis come from the LLVM build, which a standalone
# llvm-msil does not sit next to.
LLVM_BIN=${LLVM_BIN:-/w/projects/llvm-cbe/llvm-project/llvm/build/build64/bin}
SDL3=${SDL3_DIR:-}

skip() { echo "sdl3-port: skipped ($*)"; exit 0; }
[ -n "$SDL3" ] || skip "SDL3_DIR unset"
[ -f "$SDL3/src/SDL.c" ] || skip "no SDL3 source at $SDL3"
[ -x "$CLANG" ] || skip "no clang at $CLANG"
[ -x "$ILASM" ] || skip "no ilasm at $ILASM"
[ -x "$LLVM_BIN/llvm-link.exe" ] || skip "no llvm-link next to $MSIL"

# SDL has to be configured first: the include tree it generates carries the
# HAVE_* answers the sources are written against.
CFG=$SDL3/build-msil
CONFIG=$CFG/include-config-release/build_config/SDL_build_config.h
if [ ! -f "$CONFIG" ]; then
  skip "SDL3 is not configured (build-msil/include-config-release missing)"
fi
INC="$CFG/include-config-release/build_config"

# The three source edits that make SDL build off its Windows assumptions. A
# source tree that already carries them is left alone.
if ! grep -q SDL_PLATFORM_MSIL "$SDL3/include/SDL3/SDL_atomic.h"; then
  (cd "$SDL3" && git apply "$HERE/sdl3-msil.patch") || {
    echo "sdl3-port: cannot apply sdl3-msil.patch"; exit 1; }
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Not every subsystem: see the header. Kept next to the sources so a reader can
# see what is in and what is out without running the script.
UNITS="
src/SDL.c src/SDL_assert.c src/SDL_error.c src/SDL_guid.c
src/SDL_hashtable.c src/SDL_hints.c src/SDL_list.c src/SDL_log.c
src/SDL_properties.c src/SDL_utils.c
"
for d in atomic audio audio/dummy core/windows cpuinfo dialog events \
         filesystem filesystem/windows haptic haptic/hidapi haptic/windows \
         io joystick joystick/hidapi libm loadso/windows locale \
         locale/windows main main/windows misc notification power \
         power/windows process render render/software render/vulkan \
         sensor sensor/windows storage stdlib thread \
         thread/windows time time/windows timer timer/windows tray \
         video video/dummy video/offscreen video/windows; do
  for f in "$SDL3/src/$d"/*.c; do
    [ -f "$f" ] && UNITS="$UNITS ${f#"$SDL3/"}"
  done
done

# The backend rejects x86 intrinsics, so SDL's SIMD paths are compiled out.
# IL is the portable target; the scalar fallbacks are what the port runs.
DEFS="-DSDL_PLATFORM_MSIL
      -DSDL_DISABLE_MMX -DSDL_DISABLE_SSE -DSDL_DISABLE_SSE2
      -DSDL_DISABLE_SSE3 -DSDL_DISABLE_SSSE3 -DSDL_DISABLE_SSE4_1
      -DSDL_DISABLE_SSE4_2 -DSDL_DISABLE_AVX -DSDL_DISABLE_AVX2
      -DSDL_DISABLE_AVX512F -DSDL_DISABLE_NEON"

i=0
OBJS=""
for f in $UNITS; do
  i=$((i + 1))
  o="$TMP/u$i"
  # The generated headers disagree with the tree about how the platform is
  # spelled, so both include roots are needed.
  if ! "$CLANG" -S -emit-llvm -O1 -w $DEFS \
       -I"$SDL3/include" -I"$SDL3/include/SDL3" -I"$INC" -I"$SDL3/src" \
       -o "$o.ll" "$SDL3/$f" 2>"$o.err"; then
    echo "sdl3-port: cannot compile ${f#$SDL3/}"
    head -3 "$o.err"
    exit 1
  fi
  OBJS="$OBJS $o.ll"
done

"$CLANG" -S -emit-llvm -O1 -w $DEFS -I"$SDL3/include" -I"$SDL3/include/SDL3" \
  -I"$INC" -I"$SDL3/src" -o "$TMP/stubs.ll" "$HERE/sdl3_stubs.c"
"$CLANG" -S -emit-llvm -O1 -w $DEFS -I"$SDL3/include" -I"$SDL3/include/SDL3" \
  -I"$INC" -I"$SDL3/src" -o "$TMP/harness.ll" "$HERE/sdl3_port.c"

# llvm-link refuses two definitions of one symbol, and SDL's own copies win.
"$LLVM_BIN/llvm-link.exe" "$TMP/harness.ll" $OBJS "$TMP/stubs.ll" \
  -o "$TMP/all.bc"

# The C runtime split is a name heuristic, so every DLL this program reaches
# has to be named. A missing one shows up as EntryPointNotFound at first call.
LIBS=""
for d in ucrtbase msvcrt user32 kernel32 gdi32 advapi32 ole32 oleaut32 \
         shell32 shcore winmm imm32 hid setupapi version winbrand; do
  LIBS="$LIBS -msil-lib=$d.dll"
done
# Without this SDL's printf and puts land in different CRTs -- printf is not an
# export of ucrtbase, so it falls through to the heuristic's msvcrt -- and the
# two CRTs buffer stdout separately, which scrambles the output order. The
# builtin libc keeps stdio inside the module, where it is one stream.
"$MSIL" "$TMP/all.bc" -o "$TMP/port.il" -msil-libc=builtin $LIBS
"$ILASM" -exe -quiet -output:"$TMP/port.exe" "$TMP/port.il"

# `dotnet` needs the framework named; a bare exe resolves mscorlib's facade.
printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
  > "$TMP/port.runtimeconfig.json"

EXPECTED="init 1
driver dummy
window 1
size 320 0
quit 1"
GOT=$(cd "$TMP" && SDL_VIDEODRIVER=dummy timeout 120 dotnet port.exe | tr -d '\r')
if [ "$GOT" != "$EXPECTED" ]; then
  echo "sdl3-port: FAILED"
  echo "--- expected ---"; echo "$EXPECTED"
  echo "--- got ---";      echo "$GOT"
  exit 1
fi
echo "sdl3-port: ok"
