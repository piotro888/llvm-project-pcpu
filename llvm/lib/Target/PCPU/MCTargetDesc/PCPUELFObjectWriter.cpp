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
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCExpr.h"

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
                                            const MCValue & Target,
                                            const MCFixup &Fixup,
                                            bool /*IsPCRel*/) const {
  unsigned Type;
  unsigned Kind = static_cast<unsigned>(Fixup.getKind());
  MCSymbolRefExpr::VariantKind Modifier = Target.getAccessVariant();
  switch (Kind) {
  case PCPU::FIXUP_PCPU_NONE:
    Type = ELF::R_PCPU_NONE;
    break; 
  case PCPU::FIXUP_PCPU_IMM: // emited for standard instruction relloc
    Type = ELF::R_PCPU_16_IMM;
    break;
  case PCPU::FIXUP_PCPU_PC: // emited for instruction reloc on pc address
    Type = ELF::R_PCPU_16_PC_INSTR;
    break;
  case MCFixupKind::FK_Data_1: // emited for standard memory content, generic data types
    Type = ELF::R_PCPU_8;
    break;
  case MCFixupKind::FK_Data_2:
   switch (Modifier) {
    default:
      llvm_unreachable("Unsupported Modifier");
    case MCSymbolRefExpr::VK_None:
      return ELF::R_PCPU_16;
    case MCSymbolRefExpr::VK_PCPU_PC_REF:
      return ELF::R_PCPU_16_PC_REF;
    }
    break;
  case MCFixupKind::FK_Data_4:
    Type = ELF::R_PCPU_32; 
    break;
  case MCFixupKind::FK_Data_8:
    Type = ELF::R_PCPU_64; 
    break; 
  default:
    llvm_unreachable("Invalid fixup kind!");
  }
  return Type;
}

bool PCPUELFObjectWriter::needsRelocateWithSymbol(const MCSymbol & /*SD*/,
                                                   unsigned Type) const {
  switch (Type) {
  default:
    return false;
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createPCPUELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<PCPUELFObjectWriter>(OSABI);
}
