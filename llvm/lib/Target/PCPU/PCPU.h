//===-- PCPU.h - Top-level interface for PCPU representation --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// PCPU back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPU_H
#define LLVM_LIB_TARGET_PCPU_PCPU_H

#include "llvm/Pass.h"

namespace llvm {
class FunctionPass;
class PCPUTargetMachine;
class PassRegistry;

// createPCPUISelDag - This pass converts a legalized DAG into a
// PCPU-specific DAG, ready for instruction scheduling.
FunctionPass *createPCPUISelDag(PCPUTargetMachine &TM);

// createPCPUDelaySlotFillerPass - This pass fills delay slots
// with useful instructions or nop's
FunctionPass *createPCPUDelaySlotFillerPass(const PCPUTargetMachine &TM);

// createPCPUMemAluCombinerPass - This pass combines loads/stores and
// arithmetic operations.
FunctionPass *createPCPUMemAluCombinerPass();

// createPCPUSetflagAluCombinerPass - This pass combines SET_FLAG and ALU
// operations.
FunctionPass *createPCPUSetflagAluCombinerPass();

void initializePCPUDAGToDAGISelPass(PassRegistry &);

} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPU_H
