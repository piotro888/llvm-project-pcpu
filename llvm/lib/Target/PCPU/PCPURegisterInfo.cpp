//===-- PCPURegisterInfo.cpp - PCPU Register Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the PCPU implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "PCPURegisterInfo.h"
#include "PCPUAluCode.h"
#include "PCPUCondCode.h"
#include "PCPUFrameLowering.h"
#include "PCPUInstrInfo.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "PCPUGenRegisterInfo.inc"

using namespace llvm;

PCPURegisterInfo::PCPURegisterInfo() : PCPUGenRegisterInfo(PCPU::RCA) {}

const uint16_t *
PCPURegisterInfo::getCalleeSavedRegs(const MachineFunction * /*MF*/) const {
  return CSR_SaveList;
}

BitVector PCPURegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  Reserved.set(PCPU::SP);
  Reserved.set(PCPU::FP); // TODO: if has fp

  return Reserved;
}

bool PCPURegisterInfo::requiresRegisterScavenging(
    const MachineFunction & /*MF*/) const {
  return true;
}

bool PCPURegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                            int SPAdj, unsigned FIOperandNum,
                                            RegScavenger *RS) const {
  llvm_unreachable("eliminate frame index");
}

bool PCPURegisterInfo::hasBasePointer(const MachineFunction &MF) const {
  return false;
}

unsigned PCPURegisterInfo::getRARegister() const { return PCPU::RCA; }

Register
PCPURegisterInfo::getFrameRegister(const MachineFunction & /*MF*/) const {
  return PCPU::FP;
}

const uint32_t *
PCPURegisterInfo::getCallPreservedMask(const MachineFunction & /*MF*/,
                                        CallingConv::ID /*CC*/) const {
  return CSR_RegMask;
}
