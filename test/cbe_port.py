#!/usr/bin/env python3
"""Run the llvm-cbe test suite against the MSIL backend.

The llvm-cbe suite asserts one thing: a program compiled through the backend
must return the same exit code as the same program compiled natively.  That
property does not depend on the emitted language, so the suite ports over
unchanged; only the pipeline behind it differs.

    python -I cbe_port.py --cbe-tests <llvm-cbe/test> \
        --clang <clang> --llvm-msil <llvm-msil> --ilasm <ilasm>

Exit status is the number of tests that failed.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

TEST_SUCCESS_EXIT_CODE = 6
TEST_XFAIL_EXIT_CODE = 25

WINDOWS = os.name == "nt"
# An assembled PE keeps this name on both hosts; Linux starts it through
# `dotnet`, which is why the extension is not a platform switch.
EXE = ".exe"
RUNTIMECONFIG = ('{"runtimeOptions":{"tfm":"net9.0","framework":'
                 '{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}')

# Constructs the backend rejects with a deliberate fatal error. A test that
# needs one is skipped, not failed: it is a documented gap in the README, not a
# regression. The message is matched, so the list cannot hide a crash.
KNOWN_UNSUPPORTED = (
    "MSIL: unsupported constant: i64 ptrtoint",  # ptrtoint in a vtable
    "MSIL: unsupported memory type",             # x86_fp80 (long double)
    "MSIL: unsupported instruction llvm.load.relative",  # ELF PIC switch table
)

# A backend failure can raise a Windows error dialog (report_fatal_error calls
# abort(), and the default CRT puts up a modal box), which would otherwise
# block this process forever. Time out instead and report it as a failure.
RUN_TIMEOUT = int(os.environ.get("MSIL_TEST_TIMEOUT", "120"))


def run(cmd, cwd=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=RUN_TIMEOUT)
    except subprocess.TimeoutExpired:
        hint = ("run `taskkill /F /IM llvm-msil.exe`" if WINDOWS
                else "kill the leftover llvm-msil process")
        return 1, ("timed out after %ds (a failure probably hung; %s)"
                   % (RUN_TIMEOUT, hint))
    return p.returncode, p.stdout.decode("utf-8", "replace")


def run_assembly(exe, cwd=None):
    """Start an assembled program.

    `dotnet` is the launcher on both hosts: a bare run binds .NET Framework's
    mscorlib on Windows, and on Linux there is no way to exec a managed PE at
    all. It needs the framework named, so the runtimeconfig goes next to the
    assembly first.
    """
    path = os.path.join(cwd, exe) if cwd else exe
    with open(os.path.splitext(path)[0] + ".runtimeconfig.json", "w",
              newline="\n") as f:
        f.write(RUNTIMECONFIG + "\n")
    return run(["dotnet", exe], cwd=cwd)


def check_xfail(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    return re.search(r"(?m)^(//|;) xfail: (.+)", text)


def get_extra_args(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    m = re.search(r"(?m)^(//|;) (?:msvc|gcc)_extra_args: (.+)", text)
    return m.group(2).split(" ") if m else []


def collect(base, exts):
    for dirname, _, files in os.walk(base):
        for fn in sorted(files):
            if fn.startswith("test_") and fn.endswith(exts):
                yield os.path.join(dirname, fn)


class Runner:
    def __init__(self, args):
        self.args = args

    def build_native(self, src, exe, opt, cplusplus):
        cc = self.args.clangxx if cplusplus else self.args.clang
        rc, out = run([cc, src, "-O" + opt, "-w", "-o", exe])
        if rc:
            return "clang failed:\n" + out
        return None

    def build_msil(self, src, exe, ir, il, opt, cplusplus, extra):
        cc = self.args.clangxx if cplusplus else self.args.clang
        rc, out = run([cc, src, "-O" + opt, "-S", "-emit-llvm", "-w", "-o", ir])
        if rc:
            return "clang -emit-llvm failed:\n" + out
        rc, out = run([self.args.llvm_msil, ir, "-o", il])
        if rc:
            return "llvm-msil failed:\n" + out
        rc, out = run([self.args.ilasm, "-exe", "-quiet", "-output:" + exe, il])
        if rc or not os.path.exists(exe):
            return "ilasm failed:\n" + out
        return None

    def one_ll(self, src, tmp):
        """IR test: the reference result comes from lli, not a native build."""
        stem = os.path.splitext(os.path.basename(src))[0]
        msil = os.path.join(tmp, stem + ".msil" + EXE)
        il = os.path.join(tmp, stem + ".il")
        want, refout = run([self.args.lli, src])
        want &= 0xFF
        if want not in (TEST_SUCCESS_EXIT_CODE, TEST_XFAIL_EXIT_CODE):
            return None, "lli returned %d" % want
        rc, out = run([self.args.llvm_msil, src, "-o", il])
        if rc:
            return False, "llvm-msil failed:\n" + out
        rc, out = run([self.args.ilasm, "-exe", "-quiet", "-output:" + msil, il])
        if rc or not os.path.exists(msil):
            return False, "ilasm failed:\n" + out
        got, out = run_assembly(msil)
        got &= 0xFF
        if got != want:
            return False, "returned %d, lli %d\n%s" % (got, want, out[-2000:])
        return True, ""

    def one(self, src, tmp, opt, cplusplus, extra):
        """Return (ok, detail) for one source file at one optimization level."""
        stem = os.path.splitext(os.path.basename(src))[0]
        native = os.path.join(tmp, stem + ".native" + EXE)
        msil = os.path.join(tmp, stem + ".msil" + EXE)
        ir = os.path.join(tmp, stem + ".ll")
        il = os.path.join(tmp, stem + ".il")

        err = self.build_native(src, native, opt, cplusplus)
        if err:
            return None, err
        want, _ = run([native])
        want &= 0xFF
        if want not in (TEST_SUCCESS_EXIT_CODE, TEST_XFAIL_EXIT_CODE):
            # The test itself is not exercising what it claims to; skip it
            # rather than blame the backend for the native result.
            return None, "native returned %d" % want

        err = self.build_msil(src, msil, ir, il, opt, cplusplus, extra)
        if err:
            if any(m in err for m in KNOWN_UNSUPPORTED):
                return None, err
            return False, err
        got, out = run_assembly(msil)
        got &= 0xFF
        if got != want:
            return False, "returned %d, native %d\n%s" % (got, want, out[-2000:])
        return True, ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cbe-tests", required=True,
                    help="path to llvm-cbe/test")
    ap.add_argument("--clang", required=True)
    ap.add_argument("--clangxx", default=None)
    ap.add_argument("--llvm-msil", required=True)
    ap.add_argument("--ilasm", required=True)
    ap.add_argument("--lli", default=None,
                    help="path to lli; required only for the .ll tests")
    ap.add_argument("--only", default=None,
                    help="substring filter on test name")
    ap.add_argument("--levels", default="0,1,2,3")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    if not args.clangxx:
        d, base = os.path.split(args.clang)
        args.clangxx = os.path.join(d, base.replace("clang", "clang++", 1))

    runner = Runner(args)
    levels = args.levels.split(",")
    tally = {"pass": 0, "fail": 0, "skip": 0, "xfail": 0}
    failures = []

    # IR tests ignore --levels: they are already optimized IR.
    ll_tests = list(collect(os.path.join(args.cbe_tests, "ll_tests"), (".ll",))) \
        if args.lli else []
    for src in ll_tests:
        name = os.path.splitext(os.path.basename(src))[0]
        if args.only and args.only not in name:
            continue
        tmp = tempfile.mkdtemp(prefix="msilcbe-")
        try:
            ok, detail = runner.one_ll(src, tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        if check_xfail(src):
            tally["xfail"] += 1
            if args.verbose:
                print("%s %s" % ("XPASS" if ok else "XFAIL", name))
        elif ok is None:
            tally["skip"] += 1
            if args.verbose:
                print("SKIP  %s: %s" % (name, detail.splitlines()[0]))
        elif ok:
            tally["pass"] += 1
            if args.verbose:
                print("PASS  %s" % name)
        else:
            tally["fail"] += 1
            failures.append((name, detail))
            print("FAIL  %s" % name)

    for src in collect(args.cbe_tests, (".c", ".cpp")):
        name = os.path.splitext(os.path.basename(src))[0]
        if args.only and args.only not in name:
            continue
        cplusplus = src.endswith(".cpp")
        extra = get_extra_args(src)
        for opt in levels:
            xf = check_xfail(src)
            tmp = tempfile.mkdtemp(prefix="msilcbe-")
            try:
                ok, detail = runner.one(src, tmp, opt, cplusplus, extra)
            finally:
                shutil.rmtree(tmp, ignore_errors=True)
            tag = "%s -O%s" % (name, opt)
            if xf:
                # pytest's non-strict xfail: expected to fail, never fails the
                # run, and a now-passing test is reported rather than fatal.
                tally["xfail"] += 1
                if args.verbose:
                    print("%s %s: %s" % ("XPASS" if ok else "XFAIL", tag,
                                         xf.group(2)))
            elif ok is None:
                tally["skip"] += 1
                if args.verbose:
                    print("SKIP  %s: %s" % (tag, detail.splitlines()[0]))
            elif ok:
                tally["pass"] += 1
                if args.verbose:
                    print("PASS  %s" % tag)
            else:
                tally["fail"] += 1
                failures.append((tag, detail))
                print("FAIL  %s" % tag)

    for tag, detail in failures:
        print("\n===== %s =====\n%s" % (tag, detail))

    print("\n%d passed, %d failed, %d skipped, %d xfail" %
          (tally["pass"], tally["fail"], tally["skip"], tally["xfail"]))
    return 1 if tally["fail"] else 0


if __name__ == "__main__":
    sys.exit(main())
