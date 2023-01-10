//===-- PCPUELFObjectWriter.cpp - PCPU ELF Writer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/PCPUBaseInfo.h"
#include "MCTargetDesc/PCPUFixupKinds.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class PCPUELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit PCPUELFObjectWriter(uint8_t OSABI);

  ~PCPUELFObjectWriter() override = default;

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
  bool needsRelocateWithSymbol(const MCSymbol &SD,
                               unsigned Type) const override;
};

} // end anonymous namespace

PCPUELFObjectWriter::PCPUELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit_=*/false, OSABI, ELF::EM_PCPU,
                              /*HasRelocationAddend_=*/true) {}

unsigned PCPUELFObjectWriter::getRelocType(MCContext & /*Ctx*/,
                                            const MCValue & /*Target*/,
                                            const MCFixup &Fixup,
                                            bool /*IsPCRel*/) const {
  unsigned Type;
  unsigned Kind = static_cast<unsigned>(Fixup.getKind());
  switch (Kind) {
  case PCPU::FIXUP_PCPU_21:
    Type = ELF::R_PCPU_21;
    break;
  case PCPU::FIXUP_PCPU_21_F:
    Type = ELF::R_PCPU_21_F;
    break;
  case PCPU::FIXUP_PCPU_25:
    Type = ELF::R_PCPU_25;
    break;
  case PCPU::FIXUP_PCPU_32:
  case FK_Data_4:
    Type = ELF::R_PCPU_32;
    break;
  case PCPU::FIXUP_PCPU_HI16:
    Type = ELF::R_PCPU_HI16;
    break;
  case PCPU::FIXUP_PCPU_LO16:
    Type = ELF::R_PCPU_LO16;
    break;
  case PCPU::FIXUP_PCPU_NONE:
    Type = ELF::R_PCPU_NONE;
    break;

  default:
    llvm_unreachable("Invalid fixup kind!");
  }
  return Type;
}

bool PCPUELFObjectWriter::needsRelocateWithSymbol(const MCSymbol & /*SD*/,
                                                   unsigned Type) const {
  switch (Type) {
  case ELF::R_PCPU_21:
  case ELF::R_PCPU_21_F:
  case ELF::R_PCPU_25:
  case ELF::R_PCPU_32:
  case ELF::R_PCPU_HI16:
    return true;
  default:
    return false;
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createPCPUELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<PCPUELFObjectWriter>(OSABI);
}
