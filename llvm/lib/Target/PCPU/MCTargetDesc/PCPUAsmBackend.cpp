//===-- PCPUAsmBackend.cpp - PCPU Assembler Backend ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PCPUFixupKinds.h"
#include "MCTargetDesc/PCPUMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Prepare value for the target space
static unsigned adjustFixupValue(unsigned Kind, uint64_t Value) {
  switch (Kind) {
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
  case PCPU::FIXUP_PCPU_IMM:
    return Value;
  case PCPU::FIXUP_PCPU_PC:
    return Value >> 2;
  default:
    llvm_unreachable("Unknown fixup kind!");
  }
}

namespace {
class PCPUAsmBackend : public MCAsmBackend {
  Triple::OSType OSType;

public:
  PCPUAsmBackend(const Target &T, Triple::OSType OST)
      : MCAsmBackend(support::little), OSType(OST) {}

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;

  // No instruction requires relaxation
  bool fixupNeedsRelaxation(const MCFixup & /*Fixup*/, uint64_t /*Value*/,
                            const MCRelaxableFragment * /*DF*/,
                            const MCAsmLayout & /*Layout*/) const override {
    return false;
  }

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

  unsigned getNumFixupKinds() const override {
    return PCPU::NumTargetFixupKinds;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override;
};

bool PCPUAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                   const MCSubtargetInfo *STI) const {
  if ((Count % 4) != 0)
    return false;

  for (uint64_t i = 0; i < Count; i += 4)
    OS.write("\x15\0\0\0", 4);

  return true;
}

void PCPUAsmBackend::applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                                 const MCValue &Target,
                                 MutableArrayRef<char> Data, uint64_t Value,
                                 bool /*IsResolved*/,
                                 const MCSubtargetInfo * /*STI*/) const {
  MCFixupKind Kind = Fixup.getKind();
  Value = adjustFixupValue(static_cast<unsigned>(Kind), Value);

  if (!Value)
    return; // This value doesn't change the encoding

  MCFixupKindInfo Info = getFixupKindInfo(Fixup.getKind());

  // The number of bits in the fixup mask
  auto NumBits = Info.TargetSize + Info.TargetOffset;
  auto NumBytes = ((NumBits+7) / 8);

  // Shift the value into position.
  Value <<= Info.TargetOffset;

  unsigned Offset = Fixup.getOffset();
  assert(Offset + NumBytes <= Data.size() && "Invalid fixup offset!");

  // For each byte of the fragment that the fixup touches, mask in the
  // bits from the fixup value.
  for (unsigned i = 0; i < NumBytes; ++i) {
    uint8_t mask = (((Value >> (i * 8)) & 0xff));
    Data[Offset + i] |= mask;
  }
}

std::unique_ptr<MCObjectTargetWriter>
PCPUAsmBackend::createObjectTargetWriter() const {
  return createPCPUELFObjectWriter(MCELFObjectTargetWriter::getOSABI(OSType));
}

const MCFixupKindInfo &
PCPUAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  static const MCFixupKindInfo Infos[PCPU::NumTargetFixupKinds] = {
      // This table *must* be in same the order of fixup_* kinds in
      // PCPUFixupKinds.h.
      // name          offset bits flags
      {"FIXUP_PCPU_NONE", 0, 16, 0},
      {"FIXUP_PCPU_IMM", 16, 16, 0},
      {"FIXUP_PCPU_PC", 16, 16, 0},
  };    

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
         "Invalid kind!");
  return Infos[Kind - FirstTargetFixupKind];
}

} // namespace

MCAsmBackend *llvm::createPCPUAsmBackend(const Target &T,
                                          const MCSubtargetInfo &STI,
                                          const MCRegisterInfo & /*MRI*/,
                                          const MCTargetOptions & /*Options*/) {
  const Triple &TT = STI.getTargetTriple();
  if (!TT.isOSBinFormatELF())
    llvm_unreachable("OS not supported");

  return new PCPUAsmBackend(T, TT.getOS());
}
