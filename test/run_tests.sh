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
LLVMBIN=$(cd "$(dirname "$MSIL")" && pwd)

CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
# The standalone build only contains llvm-msil; lli and lit come from the
# LLVM build it links against.
LLVM_BIN_DIR=${LLVM_BIN_DIR:-/w/projects/llvm-cbe/llvm-project/llvm/build/build64/bin}
if [ -x "$LLVMBIN/lli.exe" ]; then LLI=${LLI:-$LLVMBIN/lli.exe};
else LLI=${LLI:-$LLVM_BIN_DIR/lli.exe}; fi
if [ -f "$LLVMBIN/llvm-lit.cmd" ]; then LITDEFAULT=$LLVMBIN/llvm-lit.cmd;
else LITDEFAULT=$LLVM_BIN_DIR/llvm-lit.cmd; fi
LIT=${2:-${LIT:-$LITDEFAULT}}
CBE=${CBE:-/w/projects/llvm-cbe/llvm-project/llvm/projects/llvm-cbe/test}

fail=0
note() { printf '%s\n' "$*"; }

note "== lit =="
if [ -f "$LIT" ]; then
  # The lit suite needs a generated site config, so run it through the build
  # tree's lit when there is one, else fall back to plain lit.
  # The tool substitution resolves llvm-msil by name in the LLVM tool
  # directory, which is not necessarily the binary being tested; pin it.
  if "$LIT" -D "llvm-msil=$MSIL" "$HERE" > /tmp/msil-lit.log 2>&1; then
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
  if python -I "$HERE/cbe_port.py" --cbe-tests "$CBE" \
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
if [ -d "${PROGS_DIR:-$HERE/programs}" ]; then
  for p in "$PROGS_DIR"/*.sh "$HERE"/programs/*.sh; do
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
