//===-- PCPUInstrInfo.cpp - PCPU Instruction Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the PCPU implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "PCPUInstrInfo.h"
#include "MCTargetDesc/PCPUMCTargetDesc.h"
#include "PCPUAluCode.h"
#include "PCPUCondCode.h"
#include "MCTargetDesc/PCPUBaseInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "PCPUGenInstrInfo.inc"

PCPUInstrInfo::PCPUInstrInfo()
    : PCPUGenInstrInfo(PCPU::ADJCALLSTACKDOWN, PCPU::ADJCALLSTACKUP),
      RegisterInfo() {}

void PCPUInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator Position,
                                 const DebugLoc &DL,
                                 MCRegister DestinationRegister,
                                 MCRegister SourceRegister,
                                 bool KillSource) const {

  if (!PCPU::GPRRegClass.contains(DestinationRegister, SourceRegister)) {
    llvm_unreachable("Reg-to-reg copy not in GPR class");
  }

  BuildMI(MBB, Position, DL, get(PCPU::MOV), DestinationRegister)
      .addReg(SourceRegister, getKillRegState(KillSource));
}

void PCPUInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register SourceRegister, bool IsKill, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {

  DebugLoc DL;
  if (Position != MBB.end()) {
    DL = Position->getDebugLoc();
  }

  if (!PCPU::GPRRegClass.hasSubClassEq(RegisterClass)) {
    llvm_unreachable("Can't store this register to stack slot");
  }
  BuildMI(MBB, Position, DL, get(PCPU::STO))
      .addReg(SourceRegister, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

void PCPUInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register DestinationRegister, int FrameIndex,
    const TargetRegisterClass *RegisterClass,
    const TargetRegisterInfo * /*RegisterInfo*/, Register /*VReg*/) const {
  DebugLoc DL;
  if (Position != MBB.end()) {
    DL = Position->getDebugLoc();
  }

  if (!PCPU::GPRRegClass.hasSubClassEq(RegisterClass)) {
    llvm_unreachable("Can't load this register from stack slot");
  }
  BuildMI(MBB, Position, DL, get(PCPU::LDO), DestinationRegister)
      .addFrameIndex(FrameIndex)
      .addImm(0);
}

bool PCPUInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
    MachineBasicBlock &MBB = *MI.getParent();
    if (MI.getOpcode() == PCPU::PseudoCALLr) {
        // Convert pseudo indirect call to instruction sequence
        // SRL r6, 0 ; LOAD PC TO r6 (JAL RETURN ADDRESS REG)
        // ADI r6, r6, 3 ; OFFSET r6 TO POINT AT NEXT INSTRUCTION AFTER CALL (+3)
        // SRS rx, 0 ; JUMP TO ADDRESS IN SPECIFIED REGISTER
        
        Register TargetReg = MI.getOperand(0).getReg();

        BuildMI(MBB, MI, MI.getDebugLoc(), get(PCPU::SRL), PCPU::RCA).addImm(0);
        BuildMI(MBB, MI, MI.getDebugLoc(), get(PCPU::ADI), PCPU::RCA).addReg(PCPU::RCA).addImm(3);
        BuildMI(MBB, MI, MI.getDebugLoc(), get(PCPU::JIND)).addReg(TargetReg);
        
        MBB.erase(MI);
        
        return true; 
    }
    return false; // not expanded here
}


// The analyzeBranch function is used to examine conditional instructions and
// remove unnecessary instructions. This method is used by BranchFolder and
// IfConverter machine function passes to improve the CFG.
// - TrueBlock is set to the destination if condition evaluates true (it is the
//   nullptr if the destination is the fall-through branch);
// - FalseBlock is set to the destination if condition evaluates to false (it
//   is the nullptr if the branch is unconditional);
// - condition is populated with machine operands needed to generate the branch
//   to insert in insertBranch;
// Returns: false if branch could successfully be analyzed.
bool PCPUInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TrueBlock,
                                   MachineBasicBlock *&FalseBlock,
                                   SmallVectorImpl<MachineOperand> &Condition,
                                   bool AllowModify) const {
  // Iterator to current instruction being considered.
  MachineBasicBlock::iterator Instruction = MBB.end();

  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  while (Instruction != MBB.begin()) {
    --Instruction;

    // Skip over debug instructions.
    if (Instruction->isDebugInstr())
      continue;

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    if (!isUnpredicatedTerminator(*Instruction))
      break;

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!Instruction->isBranch())
      return true;

    // Handle unconditional branches.
    if (Instruction->getOpcode() == PCPU::JMP) {
      if (!AllowModify) {
        TrueBlock = Instruction->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a branch, delete them.
      MBB.erase(std::next(Instruction), MBB.end());

      Condition.clear();
      FalseBlock = nullptr;

      // Delete the jump if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(Instruction->getOperand(0).getMBB())) {
        TrueBlock = nullptr;
        Instruction->eraseFromParent();
        Instruction = MBB.end();
        continue;
      }

      // TrueBlock is used to indicate the unconditional destination.
      TrueBlock = Instruction->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches
    unsigned Opcode = Instruction->getOpcode();
    if (Opcode != PCPU::JCOND)
      return true; // Unknown opcode.

    // Multiple conditional branches are not handled here so only proceed if
    // there are no conditions enqueued.
    if (Condition.empty()) {
      LPCC::CondCode BranchCond =
          static_cast<LPCC::CondCode>(Instruction->getOperand(1).getImm());

      // TrueBlock is the target of the previously seen unconditional branch.
      FalseBlock = TrueBlock;
      TrueBlock = Instruction->getOperand(0).getMBB();
      Condition.push_back(MachineOperand::CreateImm(BranchCond));
      continue;
    }

    // Multiple conditional branches are not handled.
    return true;
  }

  // Return false indicating branch successfully analyzed.
  return false;
}

// Insert the branch with condition specified in condition and given targets
// (TrueBlock and FalseBlock). This function returns the number of machine
// instructions inserted.
unsigned PCPUInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                      MachineBasicBlock *TrueBlock,
                                      MachineBasicBlock *FalseBlock,
                                      ArrayRef<MachineOperand> Condition,
                                      const DebugLoc &DL,
                                      int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TrueBlock && "insertBranch must not be told to insert a fallthrough");
  assert(!BytesAdded && "code size not handled");

  // If condition is empty then an unconditional branch is being inserted.
  if (Condition.empty()) {
    assert(!FalseBlock && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(PCPU::JMP)).addMBB(TrueBlock);
    return 1;
  }

  // Else a conditional branch is inserted.
  assert((Condition.size() == 1) &&
         "branch conditions should have one component.");
  unsigned ConditionalCode = Condition[0].getImm();
  BuildMI(&MBB, DL, get(PCPU::JCOND)).addMBB(TrueBlock).addImm(ConditionalCode);

  // If no false block, then false behavior is fall through and no branch needs
  // to be inserted.
  if (!FalseBlock)
    return 1;

  BuildMI(&MBB, DL, get(PCPU::JMP)).addMBB(FalseBlock);
  return 2;
}

unsigned PCPUInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                      int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");

  MachineBasicBlock::iterator Instruction = MBB.end();
  unsigned Count = 0;

  while (Instruction != MBB.begin()) {
    --Instruction;
    if (Instruction->isDebugInstr())
      continue;
    if (Instruction->getOpcode() != PCPU::JMP &&
        Instruction->getOpcode() != PCPU::JCOND) {
      break;
    }

    // Remove the branch.
    Instruction->eraseFromParent();
    Instruction = MBB.end();
    ++Count;
  }

  return Count;
}

static bool getOppositeCondition(LPCC::CondCode CC, LPCC::CondCode& Result) {
  switch (CC) {
  case LPCC::ICC_T: //  true
    return false;
  case LPCC::ICC_CA:
    Result = LPCC::ICC_GEU;
    return true;
  case LPCC::ICC_EQ:
    Result = LPCC::ICC_NE;
    return true;
  case LPCC::ICC_LT:
    Result = LPCC::ICC_GE;
    return true;
  case LPCC::ICC_GT:
    Result = LPCC::ICC_LE;
    return true;
  case LPCC::ICC_LE:
    Result = LPCC::ICC_GT;
    return true;
  case LPCC::ICC_GE:
    Result = LPCC::ICC_LT;
    return true;
  case LPCC::ICC_NE:
    Result = LPCC::ICC_EQ;
    return true;
  case LPCC::ICC_OVF:
    return false;
  case LPCC::ICC_PAR:
    return false;
  case LPCC::ICC_GTU: 
    Result = LPCC::ICC_LEU;
    return true;
  case LPCC::ICC_GEU:
    Result = LPCC::ICC_CA;
    return true;
  case LPCC::ICC_LEU:
    Result = LPCC::ICC_GTU;
    return true;
  default:
    llvm_unreachable("Invalid condtional code");
  }
}

// reverseBranchCondition - Reverses the branch condition of the specified
// condition list, returning false on success and true if it cannot be
// reversed.
bool PCPUInstrInfo::reverseBranchCondition(
    SmallVectorImpl<llvm::MachineOperand> &Condition) const {
  assert((Condition.size() == 1) &&
         "Branch conditions should have one component.");

  LPCC::CondCode BranchCond =
      static_cast<LPCC::CondCode>(Condition[0].getImm());
  LPCC::CondCode ReverseCond;
  if(!getOppositeCondition(BranchCond, ReverseCond))
    return true;
  Condition[0].setImm(ReverseCond);
  return false;
}