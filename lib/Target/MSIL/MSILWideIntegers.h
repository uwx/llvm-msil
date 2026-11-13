//===-- MSILWideIntegers.h - Promote i65..i127 to i128 ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MSIL_MSILWIDEINTEGERS_H
#define LLVM_LIB_TARGET_MSIL_MSILWIDEINTEGERS_H

namespace llvm {

class Function;

/// Rewrite every integer wider than 64 and narrower than 128 bits as an i128
/// holding the value sign-extended from its real width.
bool promoteWideIntegers(Function &F);

/// Break a load or store of an integer whose store size is 3, 5, 6 or 7 bytes
/// into the power-of-two accesses the CLR has, touching only its own bytes.
bool splitOddWidthAccesses(Function &F);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MSIL_MSILWIDEINTEGERS_H
