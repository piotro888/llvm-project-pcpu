//===- PCPUCallingConv.td - Calling Conventions PCPU -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the PCPU architectures.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Return Value Calling Conventions
//===----------------------------------------------------------------------===//

// PCPU C Calling convention.
def PCPU_CCallingConv : CallingConv<[
  // Promote i8/i16 args to i32
  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3]>>,
  // Otherwise they are assigned to the stack in 4-byte aligned units.
  CCAssignToStack<4, 4>
]>;

// PCPU C return-value convention.
def PCPU_CRetConv : CallingConv<[
  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R0]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def CSR: CalleeSavedRegs<(add R4)>;
