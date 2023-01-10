//===- PCPUDisassembler.cpp - Disassembler for PCPU -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is part of the PCPU Disassembler.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_DISASSEMBLER_PCPUDISASSEMBLER_H
#define LLVM_LIB_TARGET_PCPU_DISASSEMBLER_PCPUDISASSEMBLER_H

#include "llvm/MC/MCDisassembler/MCDisassembler.h"

#define DEBUG_TYPE "PCPU-disassembler"

namespace llvm {

class PCPUDisassembler : public MCDisassembler {
public:
  PCPUDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx);

  ~PCPUDisassembler() override = default;

  // getInstruction - See MCDisassembler.
  MCDisassembler::DecodeStatus
  getInstruction(MCInst &Instr, uint64_t &Size, ArrayRef<uint8_t> Bytes,
                 uint64_t Address, raw_ostream &CStream) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_DISASSEMBLER_PCPUDISASSEMBLER_H
