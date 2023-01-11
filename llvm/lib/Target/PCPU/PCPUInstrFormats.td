//===- PCPUInstrFormats.td - PCPU Instruction Formats ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// A generic PCPU instruction.
class PCPUInst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "PCPU";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
  let Size = 4;

  field bits<32> Inst;
  field bits<32> SoftFail = 0;
}

//===----------------------------------------------------------------------===//
// Format RRRI instruction class in PCPU : <|imm|rs2|rs1|rd|opcode|>
//===----------------------------------------------------------------------===//

class PCPUFrri<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUInst<outs, ins, asmstr, pattern>
{
  bits<3>  rd;
  bits<3>  rs1;
  bits<3>  rs2;
  bits<16> imm;

  let Inst{31-16}  = imm;
  let Inst{15-13}  = rs2;
  let Inst{12-10}  = rs1;
  let Inst{9-7}    = rd;
  let Inst{6-0}    = op;
}

class PCPUFrr<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
  let imm=0; // check if this is valid
}
