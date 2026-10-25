#!/usr/bin/env bash
# Runs every check this project has, against a given llvm-msil binary.
#
#   ./run_tests.sh <path-to-llvm-msil> [cbe-tests-dir]
#
# Used as the shared correctness gate while several people work on the
# backend at once: if this passes for a worktree, that worktree has not
# regressed anything it did not mean to.

set -u

MSIL=${1:?usage: run_tests.sh <llvm-msil> [cbe-tests-dir]}
HERE=$(cd "$(dirname "$0")" && pwd)
CBE=${2:-$HERE/../../llvm-cbe/test}

CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
LLI=${LLI:-$(dirname "$MSIL")/lli.exe}
LIT=${LIT:-$(dirname "$MSIL")/llvm-lit.cmd}

fail=0
note() { printf '%s\n' "$*"; }

note "== lit =="
if [ -x "$LIT" ] || [ -f "$LIT" ]; then
  if "$LIT" "$HERE" > /tmp/msil-lit.log 2>&1; then
    note "lit: ok"
  else
    note "lit: FAILED (see /tmp/msil-lit.log)"
    tail -20 /tmp/msil-lit.log
    fail=1
  fi
else
  note "lit: skipped (no llvm-lit next to $MSIL)"
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
# The stb and PureDOOM harnesses compare against a native build rather than
# against fixed expected values, so a change that alters both identically
# still passes.
if [ -n "${PROGS:-}" ]; then
  for p in $PROGS; do
    if [ -x "$HERE/programs/$p.sh" ]; then
      if "$HERE/programs/$p.sh" "$MSIL"; then note "$p: ok"; else
        note "$p: FAILED"; fail=1
      fi
    fi
  done
fi

[ $fail -eq 0 ] && note "ALL OK" || note "FAILURES"
exit $fail
