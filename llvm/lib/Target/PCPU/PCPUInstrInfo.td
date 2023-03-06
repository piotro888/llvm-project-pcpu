//===-- PCPUInstrInfo.td - Target Description for PCPU Target -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the PCPU instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

include "PCPUInstrFormats.td"

//===----------------------------------------------------------------------===//
// PCPU Specific Node Definitions
//===----------------------------------------------------------------------===//

//  These are target-independent nodes, but have target-specific formats.
def SDT_PCPUCall         : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_PCPUCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>,
                                            SDTCisVT<1, i32>]>;
def SDT_PCPUCallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>,
                                          SDTCisVT<1, i32>]>;
def SDT_PCPUCmp : SDTypeProfile<0, 2, [SDTCisInt<0>, SDTCisSameAs<0, 1>]>;
def SDT_PCPUBrcc : SDTypeProfile<0,  2, [SDTCisVT<0, OtherVT>,
                                        SDTCisVT<1, i32>]>;

// Nodes
def PCPUcall: SDNode<"PCPUISD::CALL", SDT_PCPUCall,
                [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                SDNPVariadic]>;
def PCPUcallSeqStart     : SDNode<"ISD::CALLSEQ_START", SDT_PCPUCallSeqStart,
                              [SDNPHasChain, SDNPOutGlue]>;
def PCPUcallSeqEnd       : SDNode<"ISD::CALLSEQ_END", SDT_PCPUCallSeqEnd,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def PCPUret : SDNode<"PCPUISD::RET", SDTNone,
                [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def PCPUcmp : SDNode<"PCPUISD::CMP", SDT_PCPUCmp, [SDNPOutGlue]>;
def PCPUbrcc : SDNode<"PCPUISD::BR_CC", SDT_PCPUBrcc, [SDNPHasChain, SDNPInGlue]>;

// Types
def CallTarget : Operand<i32>;
// Branch targets have OtherVT type.
def brtarget : Operand<OtherVT> {
  let EncoderMethod = "getBranchTargetOpValue";
}
def CCOp : Operand<i32> {
  let PrintMethod = "printCCOperand";
}

//===----------------------------------------------------------------------===//
// MOVE INSTRUCTIONS
//===----------------------------------------------------------------------===//

def MOV : PCPUFr1<0b0000001,
    (outs GPR: $rd),
    (ins GPR: $rs1),
    "mov $rd, $rs1",
    [] // why? loops with definition; not specified in other targets, must be called from C++
  >;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def LDI : PCPUFi<0b0000100,
    (outs GPR: $rd),
    (ins i32imm: $imm),
    "ldi $rd, $imm",
    [(set i32 : $rd, imm : $imm)] // <<<<< bug was here imm was i32
  >;
}

// TODO: lets params

class StoreOP <dag ins, string asmstr, list<dag> pattern> :
  PCPUFrri<0b0000110, (outs), ins, asmstr, pattern> {
    let rd = 0;
}

class LoadOP <dag outs, dag ins, string asmstr, list<dag> pattern> :
  PCPUFrri<0b0000011, outs, ins, asmstr, pattern> {
    let rs2 = 0;
}

def STO: StoreOP<
  (ins GPR: $rs1, GPR: $rs2, i32imm: $imm),
  "sto $rs1, $rs2, $imm",
  [(store i32 : $rs1, (add i32 : $rs2, imm : $imm))]>;

// (store [reg]) -> (store [reg]+0)
def : Pat<(store i32:$rs1, i32:$rs2),
          (STO i32:$rs1, i32:$rs2, (i32 0))>;

def LDO : LoadOP<
    (outs GPR: $rd),
    (ins GPR: $rs1, i32imm: $imm),
    "ldo $rd, $rs1, $imm",
    [(set i32 : $rd, (load (add i32 : $rs1, imm : $imm)))]
>;

// (load [reg]) -> (load [reg]+0)
def : Pat<(i32 (load i32:$rs1)),
          (LDO i32:$rs1, (i32 0))>;

//===----------------------------------------------------------------------===//
// ARTIHMETIC instructions
//===----------------------------------------------------------------------===//

let Defs = [FREG] in {
  def ADD :
    PCPUFrr<0b0000111,  
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "add $rd, $rs1, $rs2",
      [(set i32 : $rd, 
        (add i32 : $rs1, i32 : $rs2))]
    >;

  def ADI :
    PCPUFri<0b0001000,  
      (outs GPR: $rd),
      (ins GPR: $rs1, i32imm: $imm),
      "adi $rd, $rs1, $imm",
      [(set i32 : $rd, 
        (add i32 : $rs1, imm : $imm))]
    >;
}

//===----------------------------------------------------------------------===//
// CMP instructions
//===----------------------------------------------------------------------===//

let rd = 0, Defs = [FREG] in {
  def CMP: PCPUFrr<0b0001100,
              (outs), 
              (ins GPR: $rs1, GPR: $rs2),
              "cmp $rs1, $rs2",
              [(PCPUcmp i32:$rs1, i32:$rs2)]
  >;

  def CMI: PCPUFri<0b0001101,
              (outs),
              (ins GPR: $rs1, i32imm: $imm),
              "cmp $rs1, $imm",
              [(PCPUcmp i32:$rs1, (i32 imm:$imm))]
  >; 
}

//===----------------------------------------------------------------------===//
// CALL/RET instructions
//===----------------------------------------------------------------------===//

// JUMP AND LINK
let isCall = 1, Uses = [SP], Defs = [RCA] in { //FP? 
let rd = 0x6, rs1=0 in {
def CALLi : PCPUFr1<0b0001111,
              (outs), (ins CallTarget:$k),
              "jal r6, $k", [(PCPUcall imm:$k)]>;
}
//def CALLr TODO
}

// Call patterns
def : Pat<(PCPUcall tglobaladdr:$dst), (CALLi tglobaladdr:$dst)>;
def : Pat<(PCPUcall texternalsym:$dst), (CALLi texternalsym:$dst)>;

let Defs = [SP], Uses = [SP] in {
  def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "#ADJCALLSTACKDOWN $amt1 $amt2",
                                [(PCPUcallSeqStart timm:$amt1, timm:$amt2)]>;
  def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "#ADJCALLSTACKUP $amt1 $amt2",
                                [(PCPUcallSeqEnd timm:$amt1, timm:$amt2)]>;
}

let isTerminator = 1, isReturn = 1 in {
  // FIXME: Expand to SRS with correct bits set.
  def RET : PCPUNul<0b1111111, (outs), (ins), "srs r6, 0 ; ret", [(PCPUret)]>;
}

//===----------------------------------------------------------------------===//
// JUMP/BRANCH instructions
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1 in {
  def JMP : PCPUFJMPCC<0b0001110, (outs), (ins brtarget:$target),
                  "jmp $target",
                  [(br bb:$target)]> {
    let cond = 0;
  }

  let Uses = [FREG] in {
  def JCOND : PCPUFJMPCC<0b0001110, (outs), (ins brtarget:$target, CCOp:$cond),
                         "j$cond $target",
                        [(PCPUbrcc bb:$target, imm:$cond)]>;
  }
}