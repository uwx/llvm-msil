# -*- Python -*-

# Configuration for the llvm-msil regression tests.
#
# The backend lives outside the LLVM tree, so its tests are their own lit
# suite. The LLVM lit support supplies FileCheck, %t, %s and the shell; only
# llvm-msil itself has to be added.

import os
import re

import lit.formats
import lit.llvm

config.name = "llvm-msil"
# Use lit's internal shell: an external one mangles the backslashes in the
# Windows tool paths substituted into RUN lines.
config.test_format = lit.formats.ShTest(False)
config.suffixes = [".ll"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = getattr(config, "llvm_obj_root", None)
if config.test_exec_root:
    config.test_exec_root = os.path.join(config.test_exec_root,
                                         "msil-test-exec-root")

# A generated lit.site.cfg.py fills these in; running lit straight on this
# directory does not.
if not getattr(config, "llvm_tools_dir", None):
    # lit.util.which wants a native path, so an MSYS-style /c/... one has to be
    # converted before it will find anything.
    tools = lit_config.params.get(
        "llvm_tools_dir",
        os.environ.get("LLVM_TOOLS_DIR",
                       "/w/projects/llvm-cbe/llvm-project/llvm/build/build64/bin"))
    # Non-Windows tools are never written with a drive letter, so the MSYS
    # path rewrite below applies only there.
    if os.name == "nt" and re.match(r"^/[a-zA-Z]/", tools):
        tools = tools[1].upper() + ":\\" + tools[3:].replace("/", "\\")
    config.llvm_tools_dir = tools
# The backend under test is not necessarily in the LLVM tool directory: a
# worktree builds its own llvm-msil, and the shared one is stale. run_tests.sh
# passes the directory of the binary it was given; search it first so the
# tests exercise that binary and not whichever copy happens to be installed.
msil_dir = lit_config.params.get("llvm_msil_dir", config.llvm_tools_dir)
if os.name == "nt" and re.match(r"^/[a-zA-Z]/", msil_dir):
    msil_dir = msil_dir[1].upper() + ":\\" + msil_dir[3:].replace("/", "\\")
if not getattr(config, "llvm_exe_ext", None):
    config.llvm_exe_ext = ".exe" if os.name == "nt" else ""
if lit.llvm.llvm_config is None:
    lit.llvm.initialize(lit_config, config)

llvm_config = lit.llvm.llvm_config
llvm_config.use_default_substitutions()
# use_default_substitutions only resolves tools it was given a directory for;
# when lit runs on this directory directly there is no build-tree tool dir, so
# name the executables explicitly (which also covers the .exe suffix).
llvm_config.add_tool_substitutions(
    ["llvm-msil"], [msil_dir])
llvm_config.add_tool_substitutions(
    ["FileCheck", "not", "count"], [config.llvm_tools_dir])

if not os.path.exists(os.path.join(msil_dir,
                                   "llvm-msil" + config.llvm_exe_ext)):
    config.unsupported = True
