//===-- PCPUFrameLowering.cpp - PCPU Frame Information ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the PCPU implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "PCPUFrameLowering.h"

#include "PCPUAluCode.h"
#include "PCPUInstrInfo.h"
#include "PCPUSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

void PCPUFrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
}

MachineBasicBlock::iterator PCPUFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction & /*MF*/, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}

void PCPUFrameLowering::emitEpilogue(MachineFunction & /*MF*/,
                                      MachineBasicBlock &MBB) const {
}

void PCPUFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                              BitVector &SavedRegs,
                                              RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
}

bool PCPUFrameLowering::hasFP(const MachineFunction &MF) const {
  return true;
}
