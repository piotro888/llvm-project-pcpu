//===-- PCPUMCInstLower.h - Lower MachineInstr to MCInst -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUMCINSTLOWER_H
#define LLVM_LIB_TARGET_PCPU_PCPUMCINSTLOWER_H

#include "llvm/Support/Compiler.h"

namespace llvm {
class AsmPrinter;
class MCContext;
class MCInst;
class MCOperand;
class MCSymbol;
class MachineInstr;
class MachineOperand;

// PCPUMCInstLower - This class is used to lower an MachineInstr
// into an MCInst.
class LLVM_LIBRARY_VISIBILITY PCPUMCInstLower {
  MCContext &Ctx;

  AsmPrinter &Printer;

public:
  PCPUMCInstLower(MCContext &CTX, AsmPrinter &AP) : Ctx(CTX), Printer(AP) {}
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;

  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;

  MCSymbol *GetGlobalAddressSymbol(const MachineOperand &MO) const;
  MCSymbol *GetBlockAddressSymbol(const MachineOperand &MO) const;
  MCSymbol *GetExternalSymbolSymbol(const MachineOperand &MO) const;
  MCSymbol *GetJumpTableSymbol(const MachineOperand &MO) const;
  MCSymbol *GetConstantPoolIndexSymbol(const MachineOperand &MO) const;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUMCINSTLOWER_H
