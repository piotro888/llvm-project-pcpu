//===-- PCPUTargetMachine.h - Define TargetMachine for PCPU --- C++ ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the PCPU specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUTARGETMACHINE_H
#define LLVM_LIB_TARGET_PCPU_PCPUTARGETMACHINE_H

#include "PCPUISelLowering.h"
#include "PCPUInstrInfo.h"
#include "PCPUSelectionDAGInfo.h"
#include "PCPUSubtarget.h"
#include "llvm/Target/TargetMachine.h"
#include <optional>

namespace llvm {

class PCPUTargetMachine : public LLVMTargetMachine {
  PCPUSubtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  PCPUTargetMachine(const Target &TheTarget, const Triple &TargetTriple,
                     StringRef Cpu, StringRef FeatureString,
                     const TargetOptions &Options,
                     std::optional<Reloc::Model> RM,
                     std::optional<CodeModel::Model> CodeModel,
                     CodeGenOpt::Level OptLevel, bool JIT);

  const PCPUSubtarget *
  getSubtargetImpl(const llvm::Function & /*Fn*/) const override {
    return &Subtarget;
  }

  TargetTransformInfo getTargetTransformInfo(const Function &F) const override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &pass_manager) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  MachineFunctionInfo *
  createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F,
                            const TargetSubtargetInfo *STI) const override;

  bool isMachineVerifierClean() const override {
    return false;
  }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUTARGETMACHINE_H
