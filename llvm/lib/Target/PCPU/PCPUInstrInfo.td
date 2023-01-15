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

def PCPUret : SDNode<"PCPUISD::RET", SDTNone,
                [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

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

// TODO: lets params, subclass
let rd=0 in {
def STO : PCPUFrri<0b0000110,
    (outs),
    (ins GPR: $rs1, GPR: $rs2, i32imm: $imm),
    "sto $rs1, $rs2, $imm",
    [(store i32 : $rs1, (add i32 : $rs2, imm : $imm))]
  >;
}

let rs2=0 in {
def LDO : PCPUFrri<0b0000011,
    (outs GPR: $rd),
    (ins GPR: $rs1, i32imm: $imm),
    "ldo $rd, $rs1, $imm",
    [(set i32 : $rd, (load (add i32 : $rs1, imm : $imm)))]
  >;
}

// TODO: lets params, subclass
// TODO: replace with STO/LDO, lower/select?
let rd=0, imm=0 in {
def STDo : PCPUFrri<0b1000110,
    (outs),
    (ins GPR: $rs1, GPR: $rs2),
    "std $rs1, $rs2, <0>",
    [(store i32 : $rs1, i32 : $rs2)]
  >;
}

let rs2=0, imm=0 in {
def LDDo : PCPUFrri<0b1000011,
    (outs GPR: $rd),
    (ins GPR: $rs1),
    "ldd $rd, $rs1, <0>",
    [(set i32 : $rd, (load i32 : $rs1))]
  >;
}

//===----------------------------------------------------------------------===//
// ARTIHMETIC instructions
//===----------------------------------------------------------------------===//

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

//===----------------------------------------------------------------------===//
// CALL/RET instructions
//===----------------------------------------------------------------------===//

let isTerminator = 1, isReturn = 1 in {
  // FIXME: Expand to SRS with correct bits set.
  def RET : PCPUNul<0b1111111, (outs), (ins), "srs r6, 0 ; ret", [(PCPUret)]>;
}
