//===-- PCPUInstrInfo.cpp - PCPU Instruction Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the PCPU implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "PCPUInstrInfo.h"
#include "PCPUAluCode.h"
#include "PCPUCondCode.h"
#include "MCTargetDesc/PCPUBaseInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "PCPUGenInstrInfo.inc"

PCPUInstrInfo::PCPUInstrInfo()
    : PCPUGenInstrInfo(),
      RegisterInfo() {}

void PCPUInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator Position,
                                 const DebugLoc &DL,
                                 MCRegister DestinationRegister,
                                 MCRegister SourceRegister,
                                 bool KillSource) const {

  if (!PCPU::GPRRegClass.contains(DestinationRegister, SourceRegister)) {
    llvm_unreachable("Reg-to-reg copy not in GPR class");
  }

  BuildMI(MBB, Position, DL, get(PCPU::MOV), DestinationRegister)
      .addReg(SourceRegister, getKillRegState(KillSource));
}

void PCPUInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register SourceRegister, bool IsKill, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {

  DebugLoc DL;
  if (Position != MBB.end()) {
    DL = Position->getDebugLoc();
  }
  llvm_unreachable("test");

  if (!PCPU::GPRRegClass.hasSubClassEq(RegisterClass)) {
    llvm_unreachable("Can't store this register to stack slot");
  }
  BuildMI(MBB, Position, DL, get(PCPU::STO))
      .addReg(SourceRegister, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

void PCPUInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register DestinationRegister, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  DebugLoc DL;
  llvm_unreachable("test");
  if (Position != MBB.end()) {
    DL = Position->getDebugLoc();
  }

  if (!PCPU::GPRRegClass.hasSubClassEq(RegisterClass)) {
    llvm_unreachable("Can't load this register from stack slot");
  }
  BuildMI(MBB, Position, DL, get(PCPU::LDO), DestinationRegister)
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

