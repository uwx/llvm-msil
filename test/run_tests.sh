#!/usr/bin/env bash
# Runs every check this project has against a given llvm-msil binary.
#
#   ./run_tests.sh <path-to-llvm-msil> [lit-executable]
#
# Used as the shared correctness gate: if this passes for a worktree, that
# worktree has not regressed anything it did not mean to.
#
# Paths assume this is the checkout under llvm/projects/llvm-msil. Override
# the environment variables to point elsewhere.

set -u

MSIL=${1:?usage: run_tests.sh <llvm-msil> [lit]}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/platform.sh"
# lit runs each test from its own directory, so a relative path would not
# resolve there.
MSIL=$(cd "$(dirname "$MSIL")" && pwd)/$(basename "$MSIL")
LLVMBIN=$(cd "$(dirname "$MSIL")" && pwd)

# Windows defaults; Linux users pass LLVM_BIN_DIR and the host tools instead
# (see the README's Linux section).
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
PYTHON=${PYTHON:-python}
# The standalone build only contains llvm-msil; lli and lit come from the
# LLVM build it links against. The LLVM tarball has no lit or FileCheck, so
# those come from a separate directory on Linux (LLVM_TOOLS_DIR).
LLVM_BIN_DIR=${LLVM_BIN_DIR:-/w/projects/llvm-cbe/llvm-project/llvm/build/build64/bin}
if [ -x "$LLVMBIN/lli$EXE" ]; then LLI=${LLI:-$LLVMBIN/lli$EXE};
else LLI=${LLI:-$LLVM_BIN_DIR/lli$EXE}; fi
# lli is a native binary, so on Unix it has no suffix at all (EXE is .exe for
# the assembled output, which is a PE on both hosts).
if [ ! -x "$LLI" ] && [ -x "${LLI%.exe}" ]; then LLI=${LLI%.exe}; fi
if [ -x "$LLVMBIN/llvm-lit" ]; then LITDEFAULT=$LLVMBIN/llvm-lit;
elif [ -f "$LLVMBIN/llvm-lit.cmd" ]; then LITDEFAULT=$LLVMBIN/llvm-lit.cmd;
elif command -v lit >/dev/null 2>&1; then LITDEFAULT=$(command -v lit);
else LITDEFAULT=$LLVM_BIN_DIR/llvm-lit.cmd; fi
LIT=${2:-${LIT:-$LITDEFAULT}}
CBE=${CBE:-/w/projects/llvm-cbe/llvm-project/llvm/projects/llvm-cbe/test}
# Where FileCheck and the other LLVM utilities live, when that is not the same
# directory as lli (the release tarball ships no FileCheck; a distribution's
# llvm-NN-tools package is the usual source).
LLVM_TOOLS_DIR=${LLVM_TOOLS_DIR:-$LLVM_BIN_DIR}
export LLVM_TOOLS_DIR

fail=0
note() { printf '%s\n' "$*"; }

note "== lit =="
if [ -f "$LIT" ]; then
  # Point it at the binary under test: without this it searches the LLVM tool
  # directory, which holds a stale copy in a worktree setup.
  if "$LIT" --param llvm_tools_dir="$LLVM_TOOLS_DIR" \
       --param llvm_msil_dir="$LLVMBIN" "$HERE" > /tmp/msil-lit.log 2>&1; then
    note "lit: ok"
  else
    note "lit: FAILED (see /tmp/msil-lit.log)"
    tail -30 /tmp/msil-lit.log
    fail=1
  fi
else
  note "lit: skipped (no lit at $LIT)"
fi

note "== llvm-cbe suite =="
if [ -f "$CBE/test_cbe.py" ]; then
  if $PYTHON -I "$HERE/cbe_port.py" --cbe-tests "$CBE" \
       --clang "$CLANG" --llvm-msil "$MSIL" --ilasm "$ILASM" \
       --lli "$LLI" --levels 0,1,2,3 > /tmp/msil-cbe.log 2>&1; then
    note "llvm-cbe: $(grep -E 'passed,' /tmp/msil-cbe.log | tail -1)"
  else
    note "llvm-cbe: FAILED"
    grep -E '^FAIL|passed,' /tmp/msil-cbe.log | head -30
    fail=1
  fi
else
  note "llvm-cbe: skipped (no $CBE/test_cbe.py)"
fi

note "== real programs =="
# The program scripts resolve the same tools the same way, so hand them the
# ones chosen here instead of letting them fall back to their own defaults.
export CLANG ILASM LLVM_BIN="$LLVM_BIN_DIR"
if [ -d "${PROGS_DIR:-$HERE/programs}" ]; then
  for p in "${PROGS_DIR:-$HERE/programs}"/*.sh; do
    [ -f "$p" ] || continue
    name=$(basename "$p" .sh)
    if "$p" "$MSIL" > /tmp/msil-$name.log 2>&1; then
      note "$name: ok"
    else
      note "$name: FAILED (see /tmp/msil-$name.log)"
      tail -20 /tmp/msil-$name.log
      fail=1
    fi
  done
fi

[ $fail -eq 0 ] && note "ALL OK" || note "FAILURES"
exit $fail
