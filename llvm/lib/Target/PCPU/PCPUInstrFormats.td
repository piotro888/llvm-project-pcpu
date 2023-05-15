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

//===----------------------------------------------------------------------===//
// Subformats for Frri
//===----------------------------------------------------------------------===//

class PCPUFrr<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
  let imm=0; // check if this is valid
}

class PCPUFr1<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
  let imm=0;
  let rs2=0;
}

class PCPUFri<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
  let rs2=0;
}

class PCPUFi<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
  let rs1=0;
  let rs2=0;
}

//===----------------------------------------------------------------------===//
// Misc formats
//===----------------------------------------------------------------------===//

class PCPUFJMPCC<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>
  : PCPUInst<outs, ins, asmstr, pattern> 
{
  bits<16> target;
  bits<4>  cond;
  
  let Inst{31-16} = target;
  let Inst{9-8}   = 0;
  let Inst{10-7}  = cond;
  let Inst{6-0}   = op;
}

class PCPUNul<bits<7> op, dag outs, dag ins, string asmstr, list<dag> pattern>: 
  PCPUFrri<op, outs, ins, asmstr, pattern>
{
    // for testing only

  let imm=0;
  let rs1=0;
  let rs2=0;
  let rd=0;
}

// Pseudo instructions
class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : PCPUInst<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}
