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
  MO_NO_FLAG,

  // MO_ABS_HI/LO - Represents the hi or low part of an absolute symbol
  // address.
  MO_ABS_HI,
  MO_ABS_LO,
};
} // namespace PCPUII

static inline unsigned getPCPURegisterNumbering(unsigned Reg) {
  switch (Reg) {
  case PCPU::R0:
    return 0;
  case PCPU::R1:
    return 1;
  case PCPU::R2:
  case PCPU::PC:
    return 2;
  case PCPU::R3:
    return 3;
  case PCPU::R4:
  case PCPU::SP:
    return 4;
  case PCPU::R5:
  case PCPU::FP:
    return 5;
  case PCPU::R6:
    return 6;
  case PCPU::R7:
    return 7;
  case PCPU::R8:
  case PCPU::RV:
    return 8;
  case PCPU::R9:
    return 9;
  case PCPU::R10:
  case PCPU::RR1:
    return 10;
  case PCPU::R11:
  case PCPU::RR2:
    return 11;
  case PCPU::R12:
    return 12;
  case PCPU::R13:
    return 13;
  case PCPU::R14:
    return 14;
  case PCPU::R15:
  case PCPU::RCA:
    return 15;
  case PCPU::R16:
    return 16;
  case PCPU::R17:
    return 17;
  case PCPU::R18:
    return 18;
  case PCPU::R19:
    return 19;
  case PCPU::R20:
    return 20;
  case PCPU::R21:
    return 21;
  case PCPU::R22:
    return 22;
  case PCPU::R23:
    return 23;
  case PCPU::R24:
    return 24;
  case PCPU::R25:
    return 25;
  case PCPU::R26:
    return 26;
  case PCPU::R27:
    return 27;
  case PCPU::R28:
    return 28;
  case PCPU::R29:
    return 29;
  case PCPU::R30:
    return 30;
  case PCPU::R31:
    return 31;
  default:
    llvm_unreachable("Unknown register number!");
  }
}
} // namespace llvm
#endif // LLVM_LIB_TARGET_PCPU_MCTARGETDESC_PCPUBASEINFO_H
