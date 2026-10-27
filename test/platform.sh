#!/usr/bin/env bash
# Platform facts every test script shares: what an assembled assembly is
# called, how to start it, and how to invoke ilasm.
#
#   . "$(dirname "$0")/platform.sh"    # from test/programs/*.sh
#   . "$HERE/platform.sh"              # from test/run_tests.sh
#
# Sourced, not run. Set EXE, MSIL_ILASM_FLAGS and call helpers.
#
# On Windows an assembled PE is started directly and everything is a .exe. On
# Linux there is no launcher for a managed PE: it needs a runtimeconfig.json
# next to it and `dotnet <file>`; a name with no extension is not loadable,
# so the suffix stays .exe there too.

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) MSIL_HOST=windows ;;
  *)                    MSIL_HOST=linux ;;
esac
EXE=.exe

# .NET Core's ilasm takes -exe -quiet -output:, the .NET Framework one takes
# /exe /quiet /output:. The two builds here are both Core, on both hosts.
MSIL_ILASM_OPTS="-exe -quiet"

# write_runtimeconfig <target-without-.json>
# dotnet looks for a file named after the assembly it is given. Windows-run
# assemblies do not need one (they bind the framework directly), but writing it
# unconditionally keeps this identical on both hosts.
write_runtimeconfig() {
  printf '%s\n' \
    '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
    > "$1.runtimeconfig.json"
}

# run_msil <assembly> [args...]
# Starts an assembled program. `dotnet` is the launcher on both hosts: a bare
# run binds .NET Framework's mscorlib on Windows and does not exist on Linux.
run_msil() { dotnet "$@"; }

# The C runtime the backend pinvokes when nothing else binds a symbol: msvcrt
# and ucrtbase on Windows, the one shared library on Linux. A program built
# with -msil-libc=builtin still reaches it for anything the portable library
# does not implement.
if [ "$MSIL_HOST" = windows ]; then
  MSIL_CRT_ARGS=
else
  MSIL_CRT_ARGS='-msil-lib-map=*=libc.so.6'
fi

# An ELF clang emits position-independent code by default, which turns a
# switch table into llvm.load.relative and a global address into a GOT load.
# Neither means anything in IL, which addresses through a static field, so the
# IR is built non-PIC. Accepted (and a no-op) on the Windows target too, so it
# is unconditional.
MSIL_CFLAGS=-fno-pic
