//===-- PCPUMCTargetDesc.cpp - PCPU Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides PCPU specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "PCPUMCTargetDesc.h"
#include "PCPUInstPrinter.h"
#include "PCPUMCAsmInfo.h"
#include "TargetInfo/PCPUTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>
#include <string>

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "PCPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "PCPUGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "PCPUGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createPCPUMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitPCPUMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createPCPUMCRegisterInfo(const Triple & /*TT*/) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitPCPUMCRegisterInfo(X, PCPU::RCA);
  return X;
}

static MCSubtargetInfo *
createPCPUMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";

  return createPCPUMCSubtargetInfoImpl(TT, CPUName, /*TuneCPU*/ CPUName, FS);
}

static MCStreamer *createMCStreamer(const Triple &T, MCContext &Context,
                                    std::unique_ptr<MCAsmBackend> &&MAB,
                                    std::unique_ptr<MCObjectWriter> &&OW,
                                    std::unique_ptr<MCCodeEmitter> &&Emitter,
                                    bool RelaxAll) {
  if (!T.isOSBinFormatELF())
    llvm_unreachable("OS not supported");

  return createELFStreamer(Context, std::move(MAB), std::move(OW),
                           std::move(Emitter), RelaxAll);
}

static MCInstPrinter *createPCPUMCInstPrinter(const Triple & /*T*/,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new PCPUInstPrinter(MAI, MII, MRI);
  return nullptr;
}

static MCRelocationInfo *createPCPUElfRelocation(const Triple &TheTriple,
                                                  MCContext &Ctx) {
  return createMCRelocationInfo(TheTriple, Ctx);
}

namespace {

class PCPUMCInstrAnalysis : public MCInstrAnalysis {
public:
  explicit PCPUMCInstrAnalysis(const MCInstrInfo *Info)
      : MCInstrAnalysis(Info) {}

  bool evaluateBranch(const MCInst &Inst, uint64_t Addr, uint64_t Size,
                      uint64_t &Target) const override {
    if (Inst.getNumOperands() == 0)
      return false;
    if (!isConditionalBranch(Inst) && !isUnconditionalBranch(Inst) &&
        !isCall(Inst))
      return false;

    if (Info->get(Inst.getOpcode()).OpInfo[0].OperandType ==
        MCOI::OPERAND_PCREL) {
      int64_t Imm = Inst.getOperand(0).getImm();
      Target = Addr + Size + Imm;
      return true;
    } else {
      int64_t Imm = Inst.getOperand(0).getImm();

      // Skip case where immediate is 0 as that occurs in file that isn't linked
      // and the branch target inferred would be wrong.
      if (Imm == 0)
        return false;

      Target = Imm;
      return true;
    }
  }
};

} // end anonymous namespace

static MCInstrAnalysis *createPCPUInstrAnalysis(const MCInstrInfo *Info) {
  return new PCPUMCInstrAnalysis(Info);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePCPUTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfo<PCPUMCAsmInfo> X(getThePCPUTarget());

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getThePCPUTarget(),
                                      createPCPUMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getThePCPUTarget(),
                                    createPCPUMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getThePCPUTarget(),
                                          createPCPUMCSubtargetInfo);

  // Register the MC code emitter
  TargetRegistry::RegisterMCCodeEmitter(getThePCPUTarget(),
                                        createPCPUMCCodeEmitter);

  // Register the ASM Backend
  TargetRegistry::RegisterMCAsmBackend(getThePCPUTarget(),
                                       createPCPUAsmBackend);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(getThePCPUTarget(),
                                        createPCPUMCInstPrinter);

  // Register the ELF streamer.
  TargetRegistry::RegisterELFStreamer(getThePCPUTarget(), createMCStreamer);

  // Register the MC relocation info.
  TargetRegistry::RegisterMCRelocationInfo(getThePCPUTarget(),
                                           createPCPUElfRelocation);

  // Register the MC instruction analyzer.
  TargetRegistry::RegisterMCInstrAnalysis(getThePCPUTarget(),
                                          createPCPUInstrAnalysis);
}
