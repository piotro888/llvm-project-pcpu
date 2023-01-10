//=====-- PCPUMCAsmInfo.h - PCPU asm properties -----------*- C++ -*--====//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the PCPUMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUMCASMINFO_H
#define LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class PCPUMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit PCPUMCAsmInfo(const Triple &TheTriple,
                          const MCTargetOptions &Options);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUMCASMINFO_H
