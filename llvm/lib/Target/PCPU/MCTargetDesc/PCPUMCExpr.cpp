//===-- PCPUMCExpr.cpp - PCPU specific MC expression classes ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PCPUMCExpr.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCExpr.h"


using namespace llvm;

#define DEBUG_TYPE "PCPUmcexpr"

const PCPUMCExpr *PCPUMCExpr::create(VariantKind Kind, const MCExpr *Expr,
                                       MCContext &Ctx) {
  return new (Ctx) PCPUMCExpr(Kind, Expr);
}

PCPU::Fixups PCPUMCExpr::getFixupKind() const {
  PCPU::Fixups Kind = PCPU::Fixups::LastTargetFixupKind;

  switch (getKind()) {
  case VK_PCPU_None:
    Kind = PCPU::FIXUP_PCPU_IMM;
    break;
  case VK_PCPU_PC_ADDR:
    Kind = PCPU::FIXUP_PCPU_PC;
    break;
  default:
    llvm_unreachable("Unknown vk kind");
  }

  return Kind;
}

void PCPUMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  if (Kind == VK_PCPU_None) {
    Expr->print(OS, MAI);
    return;
  }

  switch (Kind) {
  case VK_PCPU_PC_ADDR:
    Expr->print(OS, MAI);
    return;
  default:
    llvm_unreachable("Invalid kind!");
  }

  OS << '(';
  const MCExpr *Expr = getSubExpr();
  Expr->print(OS, MAI);
  OS << ')';
}

void PCPUMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

bool PCPUMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                            const MCAsmLayout *Layout,
                                            const MCFixup *Fixup) const {
  if (!getSubExpr()->evaluateAsRelocatable(Res, Layout, Fixup))
    return false;

    if(!Layout)
      return false;

    MCContext &Context = Layout->getAssembler().getContext();
    const MCSymbolRefExpr *Sym = Res.getSymA();
    MCSymbolRefExpr::VariantKind Modifier = Sym->getKind();
    if (Modifier != MCSymbolRefExpr::VK_None)
      return false;
    if (Kind == VK_PCPU_PC_ADDR) {
      Modifier = MCSymbolRefExpr::VK_PCPU_PC_REF;
    }

    Sym = MCSymbolRefExpr::create(&Sym->getSymbol(), Modifier, Context);
    Res = MCValue::get(Sym, Res.getSymB(), Res.getConstant());

  return true;
}
