//===- PCPURegisterInfo.td - PCPU Register defs ------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//  Declarations that describe the PCPU register file
//===----------------------------------------------------------------------===//

// Registers are identified with 5-bit ID numbers.
class PCPUReg<bits<5> num, string n, list<Register> subregs = [],
               list<string> altNames = []> : Register<n, altNames> {
  field bits<5> Num;
  let Num = num;
  let Namespace = "PCPU";
  let SubRegs = subregs;
}

let Namespace = "PCPU" in {
  def sub_16 : SubRegIndex<16>;
}

// Integer registers
foreach i = 0-7 in {
  def R#i : PCPUReg<i, "r"#i>, DwarfRegNum<[i]>;
}

// Register aliases
let SubRegIndices = [sub_16] in {
  def FP  : PCPUReg<5,  "fp",  [R5]>,  DwarfRegAlias<R5>;
  def RCA : PCPUReg<6,  "rc",  [R6]>,  DwarfRegAlias<R6>;
  def SP  : PCPUReg<7,  "sp",  [R7]>,  DwarfRegAlias<R7>;
}

// The order of registers represents the preferred allocation sequence.
// Register classes.
def GPR : RegisterClass<"PCPU", [i16], 16,
    (add
     R1, R2, R3, R0, R4,  // GPR 
     R6,  RCA,  // return value
     R5,  FP,  // frame pointer
     R7,  SP  // stack pointer
    )>;

// Special registers class
class PCPUSReg <bits<16> num, string n> : Register <n> {
  let Namespace = "PCPU";
  let HWEncoding = num;
}

// Flags register
def FREG : PCPUSReg<1, "SR_FLAGS">;