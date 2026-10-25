# -*- Python -*-

# Configuration for the llvm-msil regression tests.
#
# The backend lives outside the LLVM tree, so its tests are their own lit
# suite. The LLVM lit support supplies FileCheck, %t, %s and the shell; only
# llvm-msil itself has to be added.

import os

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

llvm_config = lit.llvm.llvm_config
llvm_config.use_default_substitutions()
llvm_config.add_tool_substitutions(["llvm-msil"])

if not os.path.exists(os.path.join(config.llvm_tools_dir,
                                   "llvm-msil" + config.llvm_exe_ext)):
    config.unsupported = True
