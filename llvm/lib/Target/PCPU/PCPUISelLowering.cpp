//===-- PCPUISelLowering.cpp - PCPU DAG Lowering Implementation ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the PCPUTargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "PCPUISelLowering.h"
#include "PCPU.h"
#include "PCPUCondCode.h"
#include "PCPUMachineFunctionInfo.h"
#include "PCPUSubtarget.h"
#include "PCPUTargetObjectFile.h"
#include "MCTargetDesc/PCPUBaseInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RuntimeLibcalls.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/TargetCallingConv.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>

#define DEBUG_TYPE "PCPU-lower"

using namespace llvm;

PCPUTargetLowering::PCPUTargetLowering(const TargetMachine &TM,
                                         const PCPUSubtarget &STI)
    : TargetLowering(TM) {
  // Set up the register classes.
  addRegisterClass(MVT::i16, &PCPU::GPRRegClass);

  // Compute derived properties from the register classes
  TRI = STI.getRegisterInfo();
  computeRegisterProperties(TRI);

  // Use PCPU branch codes
  setOperationAction(ISD::BR_CC, MVT::i16, Custom);

  // Expand complex branches (to sub ops like BR_CC)
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);
  setOperationAction(ISD::BRIND, MVT::Other, Custom); // note: can be disabled by setting to custom

  // We dont have select or setcc operations.
  setOperationAction(ISD::SELECT, MVT::i16, Expand);
  // Custom expand is better! => to SELECT_CC with consts
  setOperationAction(ISD::SETCC, MVT::i16, Custom);

  // Cannot automatically expand if select is exp -> pseudo
  setOperationAction(ISD::SELECT_CC, MVT::i16, Custom);

  setOperationAction(ISD::GlobalAddress, MVT::i16, Custom);

  setOperationAction(ISD::ROTL, MVT::i16, Expand);
  setOperationAction(ISD::ROTR, MVT::i16, Expand);

  // Fix load extends
  for (MVT VT : MVT::integer_valuetypes()) {
    setLoadExtAction(ISD::EXTLOAD,  VT, MVT::i1,  Promote);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1,  Promote);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1,  Promote);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i8,  Expand);
  }

  setOperationAction(ISD::UMUL_LOHI, MVT::i16, Expand);
  // need to implelemt MULHU, MULHS
  setStackPointerRegisterToSaveRestore(PCPU::SP);
}

SDValue PCPUTargetLowering::LowerOperation(SDValue Op,
                                            SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC:
    return LowerSETCC(Op, DAG);
  case ISD::BRIND:
    return LowerBRIND(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

const char *PCPUTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case PCPUISD::CALL:
    return "PCPUISD::CALL";
  case PCPUISD::RET:
    return "PCPUISD::RET";
  case PCPUISD::CMP:
    return "PCPUISD::CMP";
  case PCPUISD::BR_CC:
    return "PCPUISD::BR_CC";
  case PCPUISD::WRAPPER:
    return "PCPUISD::WRAPPER";
  case PCPUISD::SELECT_CC:
    return "PCPUISD::SELECT_CC";
  case PCPUISD::READ_SREG:
    return "PCPUISD::READ_SREG";
  case PCPUISD::WRITE_SREG:
    return "PCPUISD::WRITE_SREG";
  default:
    return nullptr;
  }
}

//===----------------------------------------------------------------------===//
//                  Call Calling Convention Implementation
//===----------------------------------------------------------------------===//
#include "PCPUGenCallingConv.inc"

SDValue PCPUTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                       SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &IsTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;

  // Tail call optimization is not supported
  IsTailCall = false;

  return LowerCCCCallTo(Chain, Callee, CallConv, IsVarArg, IsTailCall, Outs,
                          OutVals, Ins, DL, DAG, InVals);
}

// LowerCCCCallTo - functions arguments are copied from virtual regs to
// (physical regs)/(stack frame), CALLSEQ_START and CALLSEQ_END are emitted.
SDValue PCPUTargetLowering::LowerCCCCallTo(
    SDValue Chain, SDValue Callee, CallingConv::ID CallConv, bool IsVarArg,
    bool /*IsTailCall*/, const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
  MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();

  if (IsVarArg)
    CCInfo.AnalyzeCallOperands(Outs, PCPU_CCallingConv_VaArg);
  else {
    CCInfo.AnalyzeCallOperands(Outs, PCPU_CCallingConv);
  }

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getNextStackOffset();

  // Create local copies for byval args.
  SmallVector<SDValue, 4> ByValArgs;
  for (unsigned I = 0, E = Outs.size(); I != E; ++I) {
    ISD::ArgFlagsTy Flags = Outs[I].Flags;
    if (!Flags.isByVal())
      continue;

    SDValue Arg = OutVals[I];
    unsigned Size = Flags.getByValSize();
    Align Alignment = Flags.getNonZeroByValAlign();

    int FI = MFI.CreateStackObject(Size, Alignment, false);
    SDValue FIPtr = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
    SDValue SizeNode = DAG.getConstant(Size, DL, MVT::i16);

    Chain = DAG.getMemcpy(Chain, DL, FIPtr, Arg, SizeNode, Alignment,
                          /*IsVolatile=*/false,
                          /*AlwaysInline=*/false,
                          /*isTailCall=*/false, MachinePointerInfo(),
                          MachinePointerInfo());
    ByValArgs.push_back(FIPtr);
  }

  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);

  SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;
  SDValue StackPtr;

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned I = 0, J = 0, E = ArgLocs.size(); I != E; ++I) {
    CCValAssign &VA = ArgLocs[I];
    SDValue Arg = OutVals[I];
    ISD::ArgFlagsTy Flags = Outs[I].Flags;

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    default:
      llvm_unreachable("Unknown loc info!");
    }

    // Use local copy if it is a byval arg.
    if (Flags.isByVal())
      Arg = ByValArgs[J++];

    // Arguments that can be passed on register must be kept at RegsToPass
    // vector
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      assert(VA.isMemLoc());

      if (StackPtr.getNode() == nullptr)
        StackPtr = DAG.getCopyFromReg(Chain, DL, PCPU::SP,
                                      getPointerTy(DAG.getDataLayout()));

      SDValue PtrOff =
          DAG.getNode(ISD::ADD, DL, getPointerTy(DAG.getDataLayout()), StackPtr,
                      DAG.getIntPtrConstant(VA.getLocMemOffset()+2, DL));

      MemOpChains.push_back(
          DAG.getStore(Chain, DL, Arg, PtrOff, MachinePointerInfo()));
    }
  }

  // Transform all store nodes into one single node because all store nodes are
  // independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other,
                        ArrayRef<SDValue>(&MemOpChains[0], MemOpChains.size()));

  SDValue InFlag;

  // Build a sequence of copy-to-reg nodes chained together with token chain and
  // flag operands which copy the outgoing args into registers.  The InFlag in
  // necessary since all emitted instructions must be stuck together.
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I) {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[I].first,
                             RegsToPass[I].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  if (G) {
    Callee = DAG.getTargetGlobalAddress(
        G->getGlobal(), DL, getPointerTy(DAG.getDataLayout()), 0);
  } else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(
        E->getSymbol(), getPointerTy(DAG.getDataLayout()));
  }

  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add a register mask operand representing the call-preserved registers.
  // TODO: Should return-twice functions be handled?
  const uint32_t *Mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  // Add argument registers to the end of the list so that they are
  // known live into the call.
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I)
    Ops.push_back(DAG.getRegister(RegsToPass[I].first,
                                  RegsToPass[I].second.getValueType()));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  Chain = DAG.getNode(PCPUISD::CALL, DL, NodeTys,
                      ArrayRef<SDValue>(&Ops[0], Ops.size()));
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, InFlag, DL);
  InFlag = Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, IsVarArg, Ins, DL, DAG,
                         InVals);
}

// PCPUCallResult - Lower the result values of a call into the
// appropriate copies out of appropriate physical registers.
SDValue PCPUTargetLowering::LowerCallResult(
    SDValue Chain, SDValue InFlag, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 4> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, PCPU_CRetConv);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned I = 0; I != RVLocs.size(); ++I) {
    Chain = DAG.getCopyFromReg(Chain, DL, RVLocs[I].getLocReg(),
                               RVLocs[I].getValVT(), InFlag)
                .getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }

  return Chain;
}

SDValue PCPUTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {

  if (CallConv != CallingConv::C)
    report_fatal_error("Unsupported calling convention");
  return LowerCCCArguments(Chain, CallConv, IsVarArg, Ins, DL, DAG, InVals);
}


// TODO: Return struct and VarArgs
// LowerCCCArguments - transform physical registers into virtual registers and
// generate load operations for arguments places on the stack.
SDValue PCPUTargetLowering::LowerCCCArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  PCPUMachineFunctionInfo *PCPUMFI = MF.getInfo<PCPUMachineFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 8> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins, PCPU_CCallingConv);

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    if (VA.isRegLoc()) {
      // Arguments passed in registers
      EVT RegVT = VA.getLocVT();
      switch (RegVT.getSimpleVT().SimpleTy) {
      case MVT::i16: {
        Register VReg = RegInfo.createVirtualRegister(&PCPU::GPRRegClass);
        RegInfo.addLiveIn(VA.getLocReg(), VReg);
        SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, VReg, RegVT);

        // If this is an 8-bit value, it is really passed promoted to 16
        // bits. Insert an assert[sz]ext to capture this, then truncate to the
        // right size.
        if (VA.getLocInfo() == CCValAssign::SExt)
          ArgValue = DAG.getNode(ISD::AssertSext, DL, RegVT, ArgValue,
                                 DAG.getValueType(VA.getValVT()));
        else if (VA.getLocInfo() == CCValAssign::ZExt)
          ArgValue = DAG.getNode(ISD::AssertZext, DL, RegVT, ArgValue,
                                 DAG.getValueType(VA.getValVT()));

        if (VA.getLocInfo() != CCValAssign::Full)
          ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);

        InVals.push_back(ArgValue);
        break;
      }
      default:
        LLVM_DEBUG(dbgs() << "LowerFormalArguments Unhandled argument type: "
                          << RegVT.getEVTString() << "\n");
        llvm_unreachable("unhandled argument type");
      }
    } else {
      // Only arguments passed on the stack should make it here.
      assert(VA.isMemLoc());
      // Load the argument to a virtual register
      unsigned ObjSize = VA.getLocVT().getSizeInBits() / 8;
      // Check that the argument fits in stack slot
      if (ObjSize > 2) {
        errs() << "LowerFormalArguments Unhandled argument type: "
               << EVT(VA.getLocVT()).getEVTString() << "\n";
      }
      // Create the frame index object for this incoming parameter...
      // +2 because SP points to first free address, which we cant take, decrement it by one index
      // collison with locals is fixed at computing stack size in lowering
      int FI = MFI.CreateFixedObject(ObjSize, VA.getLocMemOffset()+2, true);

      // Create the SelectionDAG nodes corresponding to a load
      // from this parameter
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i16);
      InVals.push_back(DAG.getLoad(
          VA.getLocVT(), DL, Chain, FIN,
          MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI)));
    }
  }

  // // The Lanai ABI for returning structs by value requires that we copy
  // // the sret argument into rv for the return. Save the argument into
  // // a virtual register so that we can access it from the return points.
  // if (MF.getFunction().hasStructRetAttr()) {
  //   Register Reg = LanaiMFI->getSRetReturnReg();
  //   if (!Reg) {
  //     Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(MVT::i16));
  //     LanaiMFI->setSRetReturnReg(Reg);
  //   }
  //   SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), DL, Reg, InVals[0]);
  //   Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Copy, Chain);
  // }

  // if (IsVarArg) {
  //   // Record the frame index of the first variable argument
  //   // which is a value necessary to VASTART.
  //   int FI = MFI.CreateFixedObject(4, CCInfo.getNextStackOffset(), true);
  //   LanaiMFI->setVarArgsFrameIndex(FI);
  // }

  return Chain;
}

SDValue
PCPUTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                 bool IsVarArg,
                                 const SmallVectorImpl<ISD::OutputArg> &Outs,
                                 const SmallVectorImpl<SDValue> &OutVals,
                                 const SDLoc &DL, SelectionDAG &DAG) const {
  // CCValAssign - represent the assignment of the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  // Analize return values.
  CCInfo.AnalyzeReturn(Outs, PCPU_CCallingConv);

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[i], Flag);

    // Guarantee that all emitted copies are stuck together with flags.
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // // The Lanai ABI for returning structs by value requires that we copy
  // // the sret argument into rv for the return. We saved the argument into
  // // a virtual register in the entry block, so now we copy the value out
  // // and into rv.
  // if (DAG.getMachineFunction().getFunction().hasStructRetAttr()) {
  //   MachineFunction &MF = DAG.getMachineFunction();
  //   PCPUMachineFunctionInfo *PCPUMFI = MF.getInfo<PCPUMachineFunctionInfo>();
  //   Register Reg = PCPUMFI->getSRetReturnReg();
  //   assert(Reg &&
  //          "SRetReturnReg should have been set in LowerFormalArguments().");
  //   SDValue Val =
  //       DAG.getCopyFromReg(Chain, DL, Reg, getPointerTy(DAG.getDataLayout()));

  //   Chain = DAG.getCopyToReg(Chain, DL, PCPU::R0, Val, Flag);
  //   Flag = Chain.getValue(1);
  //   RetOps.push_back(
  //       DAG.getRegister(PCPU::R0, getPointerTy(DAG.getDataLayout())));
  // }

  RetOps[0] = Chain; // Update chain

  unsigned Opc = PCPUISD::RET;
  if (Flag.getNode())
    RetOps.push_back(Flag);

  // Return Void
  return DAG.getNode(Opc, DL, MVT::Other,
                     ArrayRef<SDValue>(&RetOps[0], RetOps.size()));
}

//===----------------------------------------------------------------------===//
//                             Misc
//===----------------------------------------------------------------------===//

std::pair<unsigned, const TargetRegisterClass *>
PCPUTargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                  StringRef Constraint,
                                                  MVT VT) const {
  if (Constraint.size() == 1)
    // GCC Constraint Letters
    switch (Constraint[0]) {
    case 'r': // GENERAL_REGS
      return std::make_pair(0U, &PCPU::GPRRegClass);
    default:
      break;
    }

  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}


//===----------------------------------------------------------------------===//
//                             Custom Lowerings
//===----------------------------------------------------------------------===//

// Translate ISD cond code into PCPU branch code
static LPCC::CondCode IntCondCCodeToICC(SDValue CC, const SDLoc &DL,
                                        SDValue &RHS, SelectionDAG &DAG) {
  ISD::CondCode SetCCOpcode = cast<CondCodeSDNode>(CC)->get();

  // Only integer comparasions are supported
  switch (SetCCOpcode) {
  case ISD::SETEQ:
    return LPCC::ICC_EQ;
  case ISD::SETGT:
    return LPCC::ICC_GT;
  case ISD::SETUGT:
    return LPCC::ICC_GTU;
  case ISD::SETLT:
    return LPCC::ICC_LT;
  case ISD::SETULT:
    return LPCC::ICC_CA; // LTU->CA
  case ISD::SETLE:
    return LPCC::ICC_LE;
  case ISD::SETULE:
    return LPCC::ICC_LEU;
  case ISD::SETGE:
    return LPCC::ICC_GE;
  case ISD::SETUGE:
    return LPCC::ICC_GEU;
  case ISD::SETNE:
    return LPCC::ICC_NE;
  case ISD::SETONE:
  case ISD::SETUNE:
  case ISD::SETOGE:
  case ISD::SETOLE:
  case ISD::SETOLT:
  case ISD::SETOGT:
  case ISD::SETOEQ:
  case ISD::SETUEQ:
  case ISD::SETO:
  case ISD::SETUO:
    llvm_unreachable("Unsupported comparison.");
  default:
    llvm_unreachable("Unknown integer condition code!");
  }
}

SDValue PCPUTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  SDValue Cond = Op.getOperand(1);
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc DL(Op);

  LPCC::CondCode CC = IntCondCCodeToICC(Cond, DL, RHS, DAG);
  SDValue TargetCC = DAG.getConstant(CC, DL, MVT::i16);
  SDValue Flag = DAG.getNode(PCPUISD::CMP, DL, MVT::Glue, LHS, RHS, TargetCC);

  return DAG.getNode(PCPUISD::BR_CC, DL, Op.getValueType(), Chain, Dest,
                     TargetCC, Flag);
}

SDValue PCPUTargetLowering::LowerGlobalAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  auto DL = DAG.getDataLayout();

  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();

  // Create the TargetGlobalAddress node, folding in the constant offset.
  SDValue Result =
      DAG.getTargetGlobalAddress(GV, SDLoc(Op), getPointerTy(DL), Offset);
  return DAG.getNode(PCPUISD::WRAPPER, SDLoc(Op), getPointerTy(DL), Result);
}


SDValue PCPUTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue CC = Op.getOperand(2);
  SDLoc DL(Op);

  // SETCC is just a SELECT_CC(0, 1, CC). Custom expand eliminates need for LookThroughSetCC,
  // which eliminates additonal CMP in SELECT_CC from automatic Expand (it creates extra CMP
  // glued to SELECT_CC(0,1,CC) producing one additonal compare

  SDValue Flag = DAG.getNode(PCPUISD::CMP, DL, MVT::Glue, LHS, RHS);
  LPCC::CondCode PCPUCC = IntCondCCodeToICC(CC, DL, RHS, DAG);

  SDValue TrueV = DAG.getConstant(1, DL, Op.getValueType());
  SDValue FalseV = DAG.getConstant(0, DL, Op.getValueType());

  return DAG.getNode(PCPUISD::SELECT_CC, DL, TrueV.getValueType(), TrueV, FalseV,
        DAG.getConstant(PCPUCC, DL, MVT::i16), Flag);
}

SDValue PCPUTargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue CC = Op.getOperand(4);
  SDValue TrueVal = Op.getOperand(2);
  SDValue FalseVal = Op.getOperand(3);
  SDLoc DL(Op);

  SDValue Flag = DAG.getNode(PCPUISD::CMP, DL, MVT::Glue, LHS, RHS);
  LPCC::CondCode TCC = IntCondCCodeToICC(CC, DL, RHS, DAG);

  return DAG.getNode(PCPUISD::SELECT_CC, DL, TrueVal.getValueType(), TrueVal, FalseVal,
        DAG.getConstant(TCC, DL, MVT::i16), Flag);
}

MachineBasicBlock* PCPUTargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                 MachineBasicBlock *BB) const {
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unknown custom inserter!");
  case PCPU::SELECT_CC_PSEUDO:
    return expandSelectCC(MI, BB);
  }
}

MachineBasicBlock* PCPUTargetLowering::expandSelectCC(MachineInstr &MI, MachineBasicBlock *BB) const {
  const PCPUInstrInfo &TII = (const PCPUInstrInfo &)*MI.getParent()
                                ->getParent()
                                ->getSubtarget()
                                .getInstrInfo();
  DebugLoc dl = MI.getDebugLoc();
  unsigned CC = (LPCC::CondCode)MI.getOperand(3).getImm();

  // To "insert" a SELECT_CC instruction, we actually have to insert the
  // triangle control-flow pattern. The incoming instruction knows the
  // destination vreg to set, the condition code register to branch on, the
  // true/false values to select between, and the condition code for the branch.
  //
  // We produce the following control flow:
  //     ThisMBB
  //     |  \
  //     |  IfFalseMBB
  //     | /
  //    SinkMBB
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator It = ++BB->getIterator();

  MachineBasicBlock *ThisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *IfFalseMBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *SinkMBB = F->CreateMachineBasicBlock(LLVM_BB);
  F->insert(It, IfFalseMBB);
  F->insert(It, SinkMBB);

  // Transfer the remainder of ThisMBB and its successor edges to SinkMBB.
  SinkMBB->splice(SinkMBB->begin(), ThisMBB,
                  std::next(MachineBasicBlock::iterator(MI)), ThisMBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(ThisMBB);

  // Set the new successors for ThisMBB.
  ThisMBB->addSuccessor(IfFalseMBB);
  ThisMBB->addSuccessor(SinkMBB);

  BuildMI(ThisMBB, dl, TII.get(PCPU::JCOND))
    .addMBB(SinkMBB)
    .addImm(CC);

  // IfFalseMBB just falls through to SinkMBB.
  IfFalseMBB->addSuccessor(SinkMBB);

  // %Result = phi [ %TrueValue, ThisMBB ], [ %FalseValue, IfFalseMBB ]
  BuildMI(*SinkMBB, SinkMBB->begin(), dl, TII.get(PCPU::PHI),
          MI.getOperand(0).getReg())
      .addReg(MI.getOperand(1).getReg())
      .addMBB(ThisMBB)
      .addReg(MI.getOperand(2).getReg())
      .addMBB(IfFalseMBB);

  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return SinkMBB;
}

SDValue PCPUTargetLowering::LowerBRIND(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  SDValue Target = Op.getOperand(1);
  SDLoc DL(Op);
  
  // srl tmp_reg, 0
  // adi tmp_reg, tmp_reg, off+4??
  // srs tmp_reg, 0
  // nope, this is indirect branch and not relative branch! (and tmp_reg is hidden behind nodes! beautyfull)
  // Target = DAG.getNode(ISD::ADD, DL, MVT::i16, Target,
  //                       DAG.getConstant(0x4, DL, MVT::i16));
  
  SDValue SREG_PC = DAG.getTargetConstant(0, DL, MVT::i16);
  return DAG.getNode(PCPUISD::WRITE_SREG, DL, MVT::Other, Chain, SREG_PC, Target);
}
