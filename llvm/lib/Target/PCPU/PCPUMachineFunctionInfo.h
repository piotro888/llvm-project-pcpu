//===- PCPUMachineFuctionInfo.h - PCPU machine func info -------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares PCPU-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_PCPU_PCPUMACHINEFUNCTIONINFO_H

#include "PCPURegisterInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

// PCPUMachineFunctionInfo - This class is derived from MachineFunction and
// contains private PCPU target-specific information for each MachineFunction.
class PCPUMachineFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

  // VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex;

public:
  
  PCPUMachineFunctionInfo(const Function &F, const TargetSubtargetInfo *STI) {}
  
  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;
  
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

};

} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUMACHINEFUNCTIONINFO_H
