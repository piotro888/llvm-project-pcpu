//===-- PCPUISelLowering.h - PCPU DAG Lowering Interface -....-*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that PCPU uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUISELLOWERING_H
#define LLVM_LIB_TARGET_PCPU_PCPUISELLOWERING_H

#include "PCPU.h"
#include "PCPURegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
/// PCPU Specific DAG Nodes
// TODO: RETURN FROM INTERRUPT see avr & dcpu
namespace PCPUISD {
enum {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  CALL, // abstract CALL
  RET,  // RET flag
  CMP, // compare two operands, set FREG
  BR_CC, // Conditional jump,
  WRAPPER, // Wraps TargetConstantPool, TargetExternalSymbol, and TargetGlobalAddress.
  SELECT_CC, // Select one of two values based on conditon. Converted to pseudo


  // ==== Fully custom ===
  // Reads value of SREG.
  // The first operand is a chain pointer. The second specifies address of the
  // required CSR. Two results are produced, the read value and the new chain
  // pointer.
  READ_SREG,
  // Write value to CSR.
  // The first operand is a chain pointer, the second specifies address of the
  // required CSR and the third is the value to write. The result is the new
  // chain pointer.
  WRITE_SREG,
};
} // namespace PCPUISD

class PCPUSubtarget;

class PCPUTargetLowering : public TargetLowering {
public:
  PCPUTargetLowering(const TargetMachine &TM, const PCPUSubtarget &STI);

  // LowerOperation - Provide custom lowering hooks for some operations.
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  // getTargetNodeName - This method returns the name of a target specific
  // DAG node.
  const char *getTargetNodeName(unsigned Opcode) const override;

  MachineBasicBlock* EmitInstrWithCustomInserter(MachineInstr &MI,
                                MachineBasicBlock *MBB) const override;

  // SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerMUL(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerSHL_PARTS(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerSRL_PARTS(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;

  // bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
  //                     bool IsVarArg,
  //                     const SmallVectorImpl<ISD::OutputArg> &Outs,
  //                     LLVMContext &Context) const override;

  // Register getRegisterByName(const char *RegName, LLT VT,
  //                            const MachineFunction &MF) const override;
  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                              StringRef Constraint, MVT VT) const override;
  // ConstraintWeight
  // getSingleConstraintMatchWeight(AsmOperandInfo &Info,
  //                                const char *Constraint) const override;
  // void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
  //                                   std::vector<SDValue> &Ops,
  //                                   SelectionDAG &DAG) const override;

  // SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  // void computeKnownBitsForTargetNode(const SDValue Op, KnownBits &Known,
  //                                    const APInt &DemandedElts,
  //                                    const SelectionDAG &DAG,
  //                                    unsigned Depth = 0) const override;

  MachineBasicBlock *expandSelectCC(MachineInstr &MI, MachineBasicBlock *BB) const;

  unsigned getJumpTableEncoding() const override;

  const MCExpr *LowerCustomJumpTableEntry(const MachineJumpTableInfo *MJTI,
                                          const MachineBasicBlock *MBB,
                                          unsigned uid,
                                          MCContext &Ctx) const override;

private:
  SDValue LowerCCCCallTo(SDValue Chain, SDValue Callee,
                         CallingConv::ID CallConv, bool IsVarArg,
                         bool IsTailCall,
                         const SmallVectorImpl<ISD::OutputArg> &Outs,
                         const SmallVectorImpl<SDValue> &OutVals,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &dl, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerCCCArguments(SDValue Chain, CallingConv::ID CallConv,
                            bool IsVarArg,
                            const SmallVectorImpl<ISD::InputArg> &Ins,
                            const SDLoc &DL, SelectionDAG &DAG,
                            SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool IsVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          const SDLoc &DL, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;

  const PCPURegisterInfo *TRI;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUISELLOWERING_H
