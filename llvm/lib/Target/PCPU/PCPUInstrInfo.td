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

def ADD :
  PCPUFrr<0b0000000,  
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