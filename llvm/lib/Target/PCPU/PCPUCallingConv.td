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
  // Promote i8 args to i16
  CCIfType<[i8, i16], CCPromoteToType<i16>>,
  CCIfType<[i16], CCAssignToReg<[R0, R1, R2, R3]>>,
  // Otherwise they are assigned to the stack in 2-byte aligned units.
  CCAssignToStack<2, 2>
]>;

// Pass all args on stack if VA_ARGS.
def PCPU_CCallingConv_VaArg : CallingConv<[
  CCAssignToStack<1, 1>,
  CCAssignToStack<2, 2>
]>;

// PCPU C return-value convention.
def PCPU_CRetConv : CallingConv<[
  CCIfType<[i8], CCPromoteToType<i16>>,
  CCIfType<[i16], CCAssignToReg<[R0]>>,
  CCIfType<[i16], CCAssignToStack<2, 2>>
]>;

def CSR: CalleeSavedRegs<(add R4)>;
