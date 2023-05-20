//===-- PCPUBaseInfo.h - Top level definitions for PCPU MC ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the PCPU target useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUBASEINFO_H
#define LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUBASEINFO_H

#include "PCPUMCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

// PCPUII - This namespace holds all of the target specific flags that
// instruction info tracks.
namespace PCPUII {
// Target Operand Flag enum.
enum TOF {
  //===------------------------------------------------------------------===//
  // PCPU Specific MachineOperand flags.
  MO_NO_FLAG
};
} // namespace PCPUII

static inline unsigned getPCPURegisterNumbering(unsigned Reg) {
  switch (Reg) {
  case PCPU::RV:
  case PCPU::R0:
    return 0;
  case PCPU::R1:
    return 1;
  case PCPU::R2:
    return 2;
  case PCPU::R3:
    return 3;
  case PCPU::R4:
    return 4;
  case PCPU::FP:
  case PCPU::R5:
    return 5;
  case PCPU::RCA:
  case PCPU::R6:
    return 6;
  case PCPU::SP:
  case PCPU::R7:
    return 7;
  default:
    llvm_unreachable("Unknown register number!");
  }
}
} // namespace llvm
#endif // LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUBASEINFO_H
