//===-- PCPUTargetMachine.cpp - Define TargetMachine for PCPU ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about PCPU target spec.
//
//===----------------------------------------------------------------------===//

#include "PCPUTargetMachine.h"

#include "PCPU.h"
#include "PCPUMachineFunctionInfo.h"
#include "PCPUTargetObjectFile.h"
#include "PCPUTargetTransformInfo.h"
#include "TargetInfo/PCPUTargetInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Target/TargetOptions.h"
#include <optional>

using namespace llvm;

namespace llvm {
void initializePCPUMemAluCombinerPass(PassRegistry &);
} // namespace llvm

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePCPUTarget() {
  // Register the target.
  RegisterTargetMachine<PCPUTargetMachine> registered_target(
      getThePCPUTarget());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializePCPUDAGToDAGISelPass(PR);
}

static std::string computeDataLayout() {
  // Data layout (keep in sync with clang/lib/Basic/Targets.cpp)
  return "E"        // Big endian
         "-m:e"     // ELF name manging
         "-p:32:32" // 32-bit pointers, 32 bit aligned
         "-i64:64"  // 64 bit integers, 64 bit aligned
         "-a:0:32"  // 32 bit alignment of objects of aggregate type
         "-n32"     // 32 bit native integer width
         "-S64";    // 64 bit natural stack alignment
}

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::PIC_);
}

PCPUTargetMachine::PCPUTargetMachine(
    const Target &T, const Triple &TT, StringRef Cpu, StringRef FeatureString,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CodeModel, CodeGenOpt::Level OptLevel,
    bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(), TT, Cpu, FeatureString, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CodeModel, CodeModel::Medium),
                        OptLevel),
      Subtarget(TT, Cpu, FeatureString, *this, Options, getCodeModel(),
                OptLevel),
      TLOF(new PCPUTargetObjectFile()) {
  initAsmInfo();
}

TargetTransformInfo
PCPUTargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(PCPUTTIImpl(this, F));
}

MachineFunctionInfo *PCPUTargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return PCPUMachineFunctionInfo::create<PCPUMachineFunctionInfo>(Allocator,
                                                                    F, STI);
}

namespace {
// PCPU Code Generator Pass Configuration Options.
class PCPUPassConfig : public TargetPassConfig {
public:
  PCPUPassConfig(PCPUTargetMachine &TM, PassManagerBase *PassManager)
      : TargetPassConfig(TM, *PassManager) {}

  PCPUTargetMachine &getPCPUTargetMachine() const {
    return getTM<PCPUTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *
PCPUTargetMachine::createPassConfig(PassManagerBase &PassManager) {
  return new PCPUPassConfig(*this, &PassManager);
}

// Install an instruction selector pass.
bool PCPUPassConfig::addInstSelector() {
  addPass(createPCPUISelDag(getPCPUTargetMachine()));
  return false;
}

// Implemented by targets that want to run passes immediately before
// machine code is emitted.
void PCPUPassConfig::addPreEmitPass() {
  addPass(createPCPUDelaySlotFillerPass(getPCPUTargetMachine()));
}

// Run passes after prolog-epilog insertion and before the second instruction
// scheduling pass.
void PCPUPassConfig::addPreSched2() {
  addPass(createPCPUMemAluCombinerPass());
}
