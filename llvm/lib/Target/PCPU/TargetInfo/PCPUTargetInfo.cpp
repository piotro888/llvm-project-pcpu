//===-- PCPUTargetInfo.cpp - PCPU Target Implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/PCPUTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getThePCPUTarget() {
  static Target ThePCPUTarget;
  return ThePCPUTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePCPUTargetInfo() {
  RegisterTarget<Triple::pcpu> X(getThePCPUTarget(), "PCPU", "PCPU",
                                  "PCPU");
}
