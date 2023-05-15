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
def SDT_PCPUCall         : SDTypeProfile<0, -1, [SDTCisVT<0, i16>]>;
def SDT_PCPUCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i16>,
                                            SDTCisVT<1, i16>]>;
def SDT_PCPUCallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i16>,
                                          SDTCisVT<1, i16>]>;
def SDT_PCPUCmp : SDTypeProfile<0, 2, [SDTCisInt<0>, SDTCisSameAs<0, 1>]>;
def SDT_PCPUBrcc : SDTypeProfile<0,  2, [SDTCisVT<0, OtherVT>,
                                        SDTCisVT<1, i16>]>;
def SDT_PCPUWrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_PCPUSelectcc : SDTypeProfile<1,  3, [SDTCisSameAs<0, 1>,
                                                  SDTCisSameAs<1, 2>,  SDTCisVT<3, i16>]>;
def SDT_PCPUReadsreg  : SDTypeProfile<1, 1, [SDTCisInt<0>, SDTCisInt<1>]>;
def SDT_PCPUWritesreg : SDTypeProfile<0, 2, [SDTCisInt<0>, SDTCisInt<1>]>;

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
def PCPUselectcc : SDNode<"PCPUISD::SELECT_CC", SDT_PCPUSelectcc, [SDNPInGlue]>;
def PCPUwrapper : SDNode<"PCPUISD::WRAPPER", SDT_PCPUWrapper>;
def PCPUreadsreg : SDNode<"PCPUISD::READ_SREG", SDT_PCPUReadsreg, [SDNPHasChain]>;
def PCPUwritesreg : SDNode<"PCPUISD::WRITE_SREG", SDT_PCPUWritesreg, [SDNPHasChain]>;

// Types
def CallTarget : Operand<i16>;
// Branch targets have OtherVT type.
def brtarget : Operand<OtherVT> {
  let EncoderMethod = "getBranchTargetOpValue";
}
def CCOp : Operand<i16> {
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
    (ins i16imm: $imm),
    "ldi $rd, $imm",
    [(set i16 : $rd, imm : $imm)] // <<<<< bug was here imm was i16
  >;
}


class StoreOP <bits<7> op, dag ins, string asmstr, list<dag> pattern> :
  PCPUFrri<op, (outs), ins, asmstr, pattern> {
    let rd = 0;
    let mayStore = 1;
}

class LoadOP <bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern> :
  PCPUFrri<op, outs, ins, asmstr, pattern> {
    let rs2 = 0;
    let mayLoad = 1;
    let canFoldAsLoad = 1;
}

def STD: StoreOP<0b0000101,
  (ins GPR: $rs1, i16imm: $imm),
  "std $rs1, $imm",
  [(store i16 : $rs1, imm : $imm)]> {
    let rs2 = 0;
}

def STO: StoreOP<0b0000110,
  (ins GPR: $rs1, GPR: $rs2, i16imm: $imm),
  "sto $rs1, $rs2, $imm",
  [(store i16 : $rs1, (add i16 : $rs2, imm : $imm))]>;

// (store [reg]) -> (store [reg]+0)
def : Pat<(store i16:$rs1, i16:$rs2),
          (STO i16:$rs1, i16:$rs2, (i16 0))>;

let isReMaterializable=1 in {
    def LDD : LoadOP<0b0000010,
      (outs GPR: $rd),
      (ins i16imm: $imm),
      "ldd $rd, $imm",
      [(set i16 : $rd, (load imm : $imm))]> {
        let rs1 = 0;
    }
}

def LDO : LoadOP<0b0000011,
    (outs GPR: $rd),
    (ins GPR: $rs1, i16imm: $imm),
    "ldo $rd, $rs1, $imm",
    [(set i16 : $rd, (load (add i16 : $rs1, imm : $imm)))]
>;

// (load [reg]) -> (load [reg]+0)
def : Pat<(i16 (load i16:$rs1)),
          (LDO i16:$rs1, (i16 0))>;

def SD8: StoreOP<0b0100001,
  (ins GPR: $rs1, i16imm: $imm),
  "sd8 $rs1, $imm",
  [(truncstorei8 i16 : $rs1, imm : $imm)]> {
    let rs2 = 0;
}

def SO8: StoreOP<0b0100010,
  (ins GPR: $rs1, GPR: $rs2, i16imm: $imm),
  "so8 $rs1, $rs2, $imm",
  [(truncstorei8 i16 : $rs1, (add i16 : $rs2, imm : $imm))]>;

// (store [reg]) -> (store [reg]+0)
def : Pat<(truncstorei8 i16:$rs1, i16:$rs2),
          (SO8 i16:$rs1, i16:$rs2, (i16 0))>;

let isReMaterializable = 1 in {
    def LD8 : LoadOP<0b0011111,
      (outs GPR: $rd),
      (ins i16imm: $imm),
      "ld8 $rd, $imm",
      [(set i16 : $rd, (zextloadi8 imm : $imm))]> {
        let rs1 = 0;
    }
}

def LO8 : LoadOP<0b0100000,
    (outs GPR: $rd),
    (ins GPR: $rs1, i16imm: $imm),
    "lo8 $rd, $rs1, $imm",
    [(set i16 : $rd, (zextloadi8 (add i16 : $rs1, imm : $imm)))]
>;

def : Pat<(i16 (zextloadi8 i16:$rs1)),
          (LO8 i16:$rs1, (i16 0))>;

/// anyext i8 -> L*8 (zext, to support SEXT i8 expand)
def : Pat<(extloadi8 i16:$rs1),
          (LO8 i16:$rs1, (i16 0))>;
def : Pat<(extloadi8 (add i16:$rs1, imm:$imm)),
          (LO8 i16:$rs1, imm:$imm)>;
def : Pat<(extloadi8 imm:$addr),
          (LD8 imm:$addr)>;

//===----------------------------------------------------------------------===//
// ARTIHMETIC instructions
//===----------------------------------------------------------------------===//

let Defs = [FREG], isAsCheapAsAMove = 1 in {
  def ADD :
    PCPUFrr<0b0000111,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "add $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (add i16 : $rs1, i16 : $rs2))]
  >;

  def ADI :
    PCPUFri<0b0001000,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "adi $rd, $rs1, $imm",
      [(set i16 : $rd,
        (add i16 : $rs1, imm : $imm))]
  >;

  // TODO::ADC, SUC
  def SUB :
    PCPUFrr<0b0001010,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "sub $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (sub i16 : $rs1, i16 : $rs2))]
  >;

  def AND :
    PCPUFrr<0b0010011,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "and $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (and i16 : $rs1, i16 : $rs2))]
  >;

  def ORR :
    PCPUFrr<0b0010100,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "orr $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (or i16 : $rs1, i16 : $rs2))]
  >;

  def XOR :
    PCPUFrr<0b0010101,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "xor $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (xor i16 : $rs1, i16 : $rs2))]
  >;

  def ANI :
    PCPUFri<0b0010110,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "ani $rd, $rs1, $imm",
      [(set i16 : $rd,
        (and i16 : $rs1, imm : $imm))]
  >;

  def ORI :
    PCPUFri<0b0010111,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "ori $rd, $rs1, $imm",
      [(set i16 : $rd,
        (or i16 : $rs1, imm : $imm))]
  >;

  def XOI :
    PCPUFri<0b0011000,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "xoi $rd, $rs1, $imm",
      [(set i16 : $rd,
        (xor i16 : $rs1, imm : $imm))]
  >;

  def SHL :
    PCPUFrr<0b0011001,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "shl $rd, $rs1, $rs2",
      [(set i16: $rd,
         (shl i16 : $rs1, i16 : $rs2))]
  >;

  def SHR :
    PCPUFrr<0b0011010,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "shr $rd, $rs1, $rs2",
      [(set i16: $rd,
         (srl i16 : $rs1, i16 : $rs2))]
  >;

  def SLI :
    PCPUFri<0b0100011,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "sli $rd, $rs1, $imm",
      [(set i16 : $rd,
        (shl i16 : $rs1, imm : $imm))]
  >;

  def SRI :
    PCPUFri<0b0100100,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "sri $rd, $rs1, $imm",
      [(set i16 : $rd,
        (srl i16 : $rs1, imm : $imm))]
  >;

  def SAR :
    PCPUFrr<0b0100101,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "sar $rd, $rs1, $rs2",
      [(set i16: $rd,
         (sra i16 : $rs1, i16 : $rs2))]
  >;

  def SAI :
    PCPUFri<0b0100110,
      (outs GPR: $rd),
      (ins GPR: $rs1, i16imm: $imm),
      "sai $rd, $rs1, $imm",
      [(set i16 : $rd,
        (sra i16 : $rs1, imm : $imm))]
  >;

  def SEX :
    PCPUFr1<0b0100111,
      (outs GPR: $rd),
      (ins GPR: $rs1),
      "sex $rd, $rs1",
      [(set i16 : $rd, (sext_inreg i16 : $rs1, i8))]
  >;
}

def MUL : // lower part mul
  PCPUFrr<0b0011100,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "mul $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (mul i16 : $rs1, i16 : $rs2))]
>;

def DIV :
  PCPUFrr<0b0011101,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "div $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (udiv i16 : $rs1, i16 : $rs2))]
>;

def MOD :
  PCPUFrr<0b0101100,
      (outs GPR: $rd),
      (ins GPR: $rs1, GPR: $rs2),
      "mod $rd, $rs1, $rs2",
      [(set i16 : $rd,
        (urem i16 : $rs1, i16 : $rs2))]
>;

//===----------------------------------------------------------------------===//
// Special instructions
//===----------------------------------------------------------------------===//

let hasNoSchedulingInfo = 1, hasSideEffects = 1, mayLoad = 0, mayStore = 0 in {
    def SRL : PCPUFri<0b0010000, 
                      (outs GPR : $rd),
                      (ins  i16imm : $imm),
                      "srl $rd, $imm",
                      [(set i16:$rd, (PCPUreadsreg i16:$imm))]
    > {let rs1=0;}

    def SRS : PCPUFri<0b0010001,
                  (outs),
                  (ins GPR : $rs1, i16imm : $imm),
                  "srs $rs1, $imm",
                  [(PCPUwritesreg i16:$imm, i16:$rs1)]
    > {let rd=0;}
}

//===----------------------------------------------------------------------===//
// CMP instructions
//===----------------------------------------------------------------------===//

let rd = 0, Defs = [FREG], isCompare = 1 in {
  def CMP: PCPUFrr<0b0001100,
              (outs),
              (ins GPR: $rs1, GPR: $rs2),
              "cmp $rs1, $rs2",
              [(PCPUcmp i16:$rs1, i16:$rs2)]
  >;

  def CMI: PCPUFri<0b0001101,
              (outs),
              (ins GPR: $rs1, i16imm: $imm),
              "cmp $rs1, $imm",
              [(PCPUcmp i16:$rs1, (i16 imm:$imm))]
  >;
}

let Uses = [FREG], usesCustomInserter = 1 in {
  def SELECT_CC_PSEUDO : Pseudo<(outs GPR:$dst), (ins GPR:$T, GPR:$F, i16imm:$cc),
            "; SELECT_CC PSEUDO!",
            [(set i16:$dst, (PCPUselectcc i16:$T, i16:$F, imm:$cc))]>;
}

//===----------------------------------------------------------------------===//
// CALL/RET instructions
//===----------------------------------------------------------------------===//

// JUMP AND LINK
let isCall = 1, Uses = [SP], Defs = [RCA] in { //FP?
    let rd = 0x6, rs1 = 0 in {
        def CALLi : PCPUFr1<0b0001111,
                      (outs), (ins CallTarget:$k),
                      "jal r6, $k", [(PCPUcall imm:$k)]>;
    }
}

// Call patterns
def : Pat<(PCPUcall tglobaladdr:$dst), (CALLi tglobaladdr:$dst)>;
def : Pat<(PCPUcall texternalsym:$dst), (CALLi texternalsym:$dst)>;

let Defs = [SP], Uses = [SP] in {
  def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i16imm:$amt1, i16imm:$amt2),
                                "#ADJCALLSTACKDOWN $amt1 $amt2",
                                [(PCPUcallSeqStart timm:$amt1, timm:$amt2)]>;
  def ADJCALLSTACKUP   : Pseudo<(outs), (ins i16imm:$amt1, i16imm:$amt2),
                                "#ADJCALLSTACKUP $amt1 $amt2",
                                [(PCPUcallSeqEnd timm:$amt1, timm:$amt2)]>;
}

let isTerminator = 1, isReturn = 1, isBarrier = 1 in {
  // FIXME: Expand to SRS with correct bits set.
  def RET : PCPUNul<0b1111111, (outs), (ins), "srs r6, 0 ; ret", [(PCPUret)]>;
}

//===----------------------------------------------------------------------===//
// JUMP/BRANCH instructions
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1 in {
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
  
  let isIndirectBranch = 1 in {
    def JIND : Pseudo<(outs), (ins GPR : $rs1), "#BRIND", [(brind GPR:$rs1)]>,
               PseudoInstExpansion<(SRS GPR:$rs1, (i16 0))>;
  }
}

//===----------------------------------------------------------------------===//
// Addressing (WRAPPER)
//===----------------------------------------------------------------------===//

// GlobalAddress
def : Pat<(i16(PCPUwrapper tglobaladdr : $dst)), (LDI tglobaladdr : $dst)>; // For long mode implementation, see AVR equivalent with DLDREGS
def : Pat<(i16(load (PCPUwrapper tglobaladdr : $dst))), (LDD tglobaladdr : $dst)>;
def : Pat<(store i16 : $src, (PCPUwrapper tglobaladdr : $dst)), (STD i16 : $src, tglobaladdr : $dst)>;
// Jump table
def : Pat<(i16(PCPUwrapper tjumptable : $jt)), (LDI tjumptable : $jt)>;
