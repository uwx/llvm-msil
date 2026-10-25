# Working on several things at once

The whole backend is one file, so parallel work collides in the same functions.
Two things make it tractable: a standalone build that takes seconds, and a
single test gate everyone agrees on.

## Setup

```
git worktree add -b wt/<topic> /w/projects/msil-wt/<topic> HEAD
cd /w/projects/msil-wt/<topic>
cmake -S . -B build -G Ninja \
      -DLLVM_DIR=W:/projects/llvm-cbe/llvm-project/llvm/build/build64/lib/cmake/llvm
cmake --build build -j 8                  # ~6 seconds
./test/run_tests.sh ./build/tools/llvm-msil/llvm-msil.exe
```

The standalone build links LLVM's already-built libraries, so it never
rebuilds LLVM. Point `LLVM_DIR` at any LLVM build tree or install.

## The gate

`test/run_tests.sh` is the contract: lit tests, the ported llvm-cbe suite (540
tests across four optimization levels), and `test/programs/*.sh`, which each
compare a real program against a native build. A change is done when the gate
is green, not when the feature appears to work.

That last part matters because the suites compare against native rather than
against fixed expected values: a change that alters both sides identically
still passes, so the gate measures "same behaviour as native", which is the
property this backend is for.

## Merging

Land one branch at a time and re-run the gate on the merged tree — a green
branch does not imply a green merge when two people changed the same helper.

```
git merge --no-ff wt/<topic>
cmake --build /tmp/msil-main -j 8
LLVM_TOOLS_DIR=/tmp/msil-main/tools/llvm-msil \
  ./test/run_tests.sh /tmp/msil-main/tools/llvm-msil/llvm-msil.exe
```

Build in a *separate* directory from the one you merge into, so the gate runs
against the merged source rather than a stale binary.

Expect conflicts in `test/run_tests.sh` in particular: it is small, frequently
touched, and everyone adds something to it, so resolve by hand rather than
taking either side.

## Gotchas

* A failing `report_fatal_error` raises `abort()`, and the Windows CRT answers
  that with a modal dialog: the process blocks instead of exiting. Wrap runs in
  a timeout. LLVM here is built with `LLVM_ENABLE_ASSERTIONS=OFF`, so `assert`
  cannot fire — the message you want is the `LLVM ERROR:` line on stderr,
  printed before the dialog appears.
* `llvm-lit` resolves `%llvm-msil` by name in the LLVM tool directory, which is
  not necessarily the binary you are testing. Pass `-D llvm-msil=<binary>` (the
  gate does this) or you will test the wrong build.
* A build directory whose `CMAKE_CXX_FLAGS_RELEASE` is empty has no `/DNDEBUG`,
  so LLVM's `assert`s are live in *our* code too, even though the LLVM library
  it links was built with assertions off. `assert` then aborts before the
  `LLVM ERROR:` line and before any dialog, and a pre-existing latent assert in
  the writer (a bad `cast<>` on some IR) looks like a regression from whatever
  you last changed. Check `grep CMAKE_CXX_FLAGS_RELEASE build/CMakeCache.txt`
  before believing a crash. The stock configure line above produces
  `/O2 /Ob2 /DNDEBUG`; if the cache was made by some other means and reads
  empty, delete `build/` and configure again. Prefer that to matching it, so
  the gate runs the same assertions configuration the release builds use.
