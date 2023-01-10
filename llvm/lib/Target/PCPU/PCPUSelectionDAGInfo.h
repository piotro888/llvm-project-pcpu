//===-- PCPUSelectionDAGInfo.h - PCPU SelectionDAG Info -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the PCPU subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_PCPU_PCPUSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class PCPUSelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  PCPUSelectionDAGInfo() = default;

  SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Dst, SDValue Src,
                                  SDValue Size, Align Alignment,
                                  bool isVolatile, bool AlwaysInline,
                                  MachinePointerInfo DstPtrInfo,
                                  MachinePointerInfo SrcPtrInfo) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUSELECTIONDAGINFO_H
