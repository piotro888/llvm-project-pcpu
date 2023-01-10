//===- PCPU.td - Describe the PCPU Target Machine --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "PCPUSchedule.td"
include "PCPURegisterInfo.td"
include "PCPUCallingConv.td"
include "PCPUInstrInfo.td"

def PCPUInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// PCPU processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic", PCPUSchedModel, []>;
def : ProcessorModel<"v11", PCPUSchedModel, []>;

def PCPUInstPrinter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def PCPU : Target {
  // Pull in Instruction Info:
  let InstructionSet = PCPUInstrInfo;
  let AssemblyWriters = [PCPUInstPrinter];
}
