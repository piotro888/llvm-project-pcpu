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
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const PCPUInstrInfo &PII =
      *static_cast<const PCPUInstrInfo *>(STI.getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  // TODO: Implement IRQ handlers
  // TODO: Implement VaArgs
  
  unsigned StackSize = MFI.getStackSize();

  // Push JAL R6 PC
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::STO))
      .addReg(PCPU::RCA)
      .addReg(PCPU::SP)
      .addImm(0)
      .setMIFlag(MachineInstr::FrameSetup);

  // Push old FP
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::STO))
      .addReg(PCPU::FP)
      .addReg(PCPU::SP)
      .addImm(4)
      .setMIFlag(MachineInstr::FrameSetup);

  // Generate new FP
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::MOV), PCPU::FP)
      .addReg(PCPU::SP)
      .setMIFlag(MachineInstr::FrameSetup);

  // Allocate space on the stack if needed
  if (StackSize != 0) {
    BuildMI(MBB, MBBI, DL, PII.get(PCPU::ADI), PCPU::SP)
        .addReg(PCPU::SP)
        .addImm(-StackSize)
        .setMIFlag(MachineInstr::FrameSetup);
  }
}

MachineBasicBlock::iterator PCPUFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction & /*MF*/, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  // Discard ADJCALLSTACKDOWN, ADJCALLSTACKUP instructions.
  return MBB.erase(I);
}

void PCPUFrameLowering::emitEpilogue(MachineFunction & /*MF*/,
                                      MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  const PCPUInstrInfo &PII =
      *static_cast<const PCPUInstrInfo *>(STI.getInstrInfo());
  DebugLoc DL = MBBI->getDebugLoc();
  
  // Restore the stack pointer using the callee's frame pointer value.
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::MOV), PCPU::SP)
      .addReg(PCPU::FP);

  // Restore the frame pointer from the stack.
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::LDO), PCPU::FP)
      .addReg(PCPU::SP)
      .addImm(-4);

  // Restore RCA (PC)
  BuildMI(MBB, MBBI, DL, PII.get(PCPU::LDO), PCPU::RCA)
      .addReg(PCPU::SP)
      .addImm(0);
}

void PCPUFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                              BitVector &SavedRegs,
                                              RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  MachineFrameInfo &MFI = MF.getFrameInfo();
  int Offset = 0;

  // Reserve 4 bytes for the saved RCA(PC)
  MFI.CreateFixedObject(4, Offset, true);
  Offset -= 4;

  // Reserve 4 bytes for the saved FP
  MFI.CreateFixedObject(4, Offset, true);
  Offset -= 4;
}

bool PCPUFrameLowering::hasFP(const MachineFunction &MF) const {
  return true; // TODO: FP elimination, see CPU0
}
