//===--- PCPU.cpp - Implement PCPU target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements PCPU TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "PCPU.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

const char *const PCPUTargetInfo::GCCRegNames[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",  "r8",  "r9",  "r10",
    "r11", "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21",
    "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

ArrayRef<const char *> PCPUTargetInfo::getGCCRegNames() const {
  return llvm::makeArrayRef(GCCRegNames);
}

const TargetInfo::GCCRegAlias PCPUTargetInfo::GCCRegAliases[] = {
    {{"pc"}, "r2"},   {{"sp"}, "r4"},   {{"fp"}, "r5"},   {{"rv"}, "r8"},
    {{"rr1"}, "r10"}, {{"rr2"}, "r11"}, {{"rca"}, "r15"},
};

ArrayRef<TargetInfo::GCCRegAlias> PCPUTargetInfo::getGCCRegAliases() const {
  return llvm::makeArrayRef(GCCRegAliases);
}

bool PCPUTargetInfo::isValidCPUName(StringRef Name) const {
  return llvm::StringSwitch<bool>(Name).Case("v11", true).Default(false);
}
void PCPUTargetInfo::fillValidCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  Values.emplace_back("v11");
}

bool PCPUTargetInfo::setCPU(const std::string &Name) {
  CPU = llvm::StringSwitch<CPUKind>(Name).Case("v11", CK_V11).Default(CK_NONE);

  return CPU != CK_NONE;
}

bool PCPUTargetInfo::hasFeature(StringRef Feature) const {
  return llvm::StringSwitch<bool>(Feature).Case("PCPU", true).Default(false);
}

void PCPUTargetInfo::getTargetDefines(const LangOptions &Opts,
                                       MacroBuilder &Builder) const {
  // Define __PCPU__ when building for target PCPU.
  Builder.defineMacro("__PCPU__");

  // Set define for the CPU specified.
  switch (CPU) {
  case CK_V11:
    Builder.defineMacro("__PCPU_V11__");
    break;
  case CK_NONE:
    llvm_unreachable("Unhandled target CPU");
  }
}
