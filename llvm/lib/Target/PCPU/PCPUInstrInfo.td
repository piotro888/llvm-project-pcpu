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

def PCPUret : SDNode<"PCPUISD::RET_FLAG", SDTNone,
                [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// Move instructions.
//===----------------------------------------------------------------------===//

def MOVrr : PCPUFr1<0b0000001,
    (outs GPR: $rd),
    (ins GPR: $rs1),
    "mov $rd, $rs1",
    [(set i32 : $rd, i32 : $rs1),
  ]>;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def MOVri : PCPUFi<0b0000010,
    (outs GPR:$rd),
    (ins i32imm:$imm),
    "mov {$rd, $imm}",
    [(set i32 : $rd, i32 : $imm)
  ]>;
}


//===----------------------------------------------------------------------===//
// Arithmetic instructions.
//===----------------------------------------------------------------------===//

def ADD :
  PCPUFrr<0b0000111,  
    (outs GPR: $rd),
    (ins GPR: $rs1, GPR: $rs2),
    "add $rd, $rs1, $rs2",
    [(set i32
      : $rd, (add i32 : $rs1,
                  i32 : $rs2)),
  ]>;

//===----------------------------------------------------------------------===//
// Return instructions.
//===----------------------------------------------------------------------===//
let isTerminator = 1, isReturn = 1 in {
  def RET : PCPUNul<0b0000001, (outs), (ins), "srs r6, 0 ; ret", [(PCPUret)]>;
}