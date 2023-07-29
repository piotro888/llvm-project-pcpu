//===-- PCPUAsmPrinter.cpp - PCPU LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the PCPU assembly language.
//
//===----------------------------------------------------------------------===//

#include "PCPUAluCode.h"
#include "PCPUCondCode.h"
#include "PCPUInstrInfo.h"
#include "PCPUMCInstLower.h"
#include "PCPUTargetMachine.h"
#include "MCTargetDesc/PCPUMCExpr.h"
#include "MCTargetDesc/PCPUInstPrinter.h"
#include "TargetInfo/PCPUTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "asm-printer"

using namespace llvm;

namespace {
class PCPUAsmPrinter : public AsmPrinter {
public:
  explicit PCPUAsmPrinter(TargetMachine &TM,
                           std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "PCPU Assembly Printer"; }

  void printOperand(const MachineInstr *MI, int OpNum, raw_ostream &O);
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &O) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI,
                                          unsigned OpNum, const char *ExtraCode,
                                          raw_ostream &O) override;
  void emitInstruction(const MachineInstr *MI) override;

  const MCExpr *lowerConstant(const Constant *CV) override;
  
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);
  // Wrapper needed for tblgenned pseudo lowering.
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp);

private:
  void customEmitInstruction(const MachineInstr *MI);
};
} // end of anonymous namespace

void PCPUAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
                                   raw_ostream &O) {
  const MachineOperand &MO = MI->getOperand(OpNum);

  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << PCPUInstPrinter::getRegisterName(MO.getReg());
    break;

  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;

  case MachineOperand::MO_MachineBasicBlock:
    O << *MO.getMBB()->getSymbol();
    break;

  case MachineOperand::MO_GlobalAddress:
    O << *getSymbol(MO.getGlobal());
    break;

  case MachineOperand::MO_BlockAddress: {
    MCSymbol *BA = GetBlockAddressSymbol(MO.getBlockAddress());
    O << BA->getName();
    break;
  }

  case MachineOperand::MO_ExternalSymbol:
    O << *GetExternalSymbolSymbol(MO.getSymbolName());
    break;

  case MachineOperand::MO_JumpTableIndex:
    O << MAI->getPrivateGlobalPrefix() << "JTI" << getFunctionNumber() << '_'
      << MO.getIndex();
    break;

  case MachineOperand::MO_ConstantPoolIndex:
    O << MAI->getPrivateGlobalPrefix() << "CPI" << getFunctionNumber() << '_'
      << MO.getIndex();
    return;

  default:
    llvm_unreachable("<unknown operand type>");
  }
}

// PrintAsmOperand - Print out an operand for an inline asm expression.
bool PCPUAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode, raw_ostream &O) {
  // Does this asm operand have a single letter operand modifier?
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1])
      return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    // The highest-numbered register of a pair.
    case 'H': {
      if (OpNo == 0)
        return true;
      const MachineOperand &FlagsOP = MI->getOperand(OpNo - 1);
      if (!FlagsOP.isImm())
        return true;
      unsigned Flags = FlagsOP.getImm();
      unsigned NumVals = InlineAsm::getNumOperandRegisters(Flags);
      if (NumVals != 2)
        return true;
      unsigned RegOp = OpNo + 1;
      if (RegOp >= MI->getNumOperands())
        return true;
      const MachineOperand &MO = MI->getOperand(RegOp);
      if (!MO.isReg())
        return true;
      Register Reg = MO.getReg();
      O << PCPUInstPrinter::getRegisterName(Reg);
      return false;
    }
    default:
      return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, O);
    }
  }
  printOperand(MI, OpNo, O);
  return false;
}

bool PCPUAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                          unsigned OpNum, const char *ExtraCode,
                                          raw_ostream &O) {
  assert(OpNum + 1 < MI->getNumOperands() && "Insufficient operands");
  const MachineOperand &BaseMO = MI->getOperand(OpNum);
  const MachineOperand &OffsetMO = MI->getOperand(OpNum + 1);
  assert(BaseMO.isReg() && "Unexpected base pointer for inline asm memory operand.");
  assert(OffsetMO.isImm() && "Unexpected offset for inline asm memory operand.");
  int Offset = OffsetMO.getImm();

  if (ExtraCode)
    return true; // Unknown modifier.

  O << PCPUInstPrinter::getRegisterName(BaseMO.getReg()) << "," << Offset;

  return false;
}

const MCExpr *PCPUAsmPrinter::lowerConstant(const Constant *CV) {
  // This is a key to PC references fixups in memory (not connected to insns)
  MCContext &Ctx = OutContext;

  if (const GlobalValue *GV = dyn_cast<GlobalValue>(CV)) {
      // program memory reference
      if(isa<Function>(GV)) {
        const MCExpr *Expr = MCSymbolRefExpr::create(getSymbol(GV), Ctx);
        return PCPUMCExpr::create(PCPUMCExpr::VK_PCPU_PC_ADDR_CONST, Expr, Ctx);
      }
  }
  return AsmPrinter::lowerConstant(CV);
}

// Simple pseudo-instructions have their lowering (with expansion to real
// instructions) auto-generated.
#include "PCPUGenMCPseudoLowering.inc"

void PCPUAsmPrinter::emitInstruction(const MachineInstr *MI) {
  PCPUMCInstLower MCInstLowering(OutContext, *this);
  MCSubtargetInfo STI = getSubtargetInfo();

  // Do any auto-generated pseudo lowerings.
  if (emitPseudoExpansionLowering(*OutStreamer, MI))
    return;

  MCInst TmpInst;
  
  MCInstLowering.Lower(MI, TmpInst);
  OutStreamer->emitInstruction(TmpInst, STI);
}

bool PCPUAsmPrinter::lowerOperand(const MachineOperand &MO, MCOperand &MCOp) {
  PCPUMCInstLower MCInstLowering(OutContext, *this);
  return MCInstLowering.lowerOperand(MO, MCOp);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePCPUAsmPrinter() {
  RegisterAsmPrinter<PCPUAsmPrinter> X(getThePCPUTarget());
}
