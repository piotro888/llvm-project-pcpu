//===-- PCPUAluCode.h - ALU operator encoding ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The encoding for ALU operators used in RM and RRM operands
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_PCPU_PCPUALUCODE_H
#define LLVM_LIB_TARGET_PCPU_PCPUALUCODE_H

#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
namespace LPAC {
enum AluCode {
  ADD = 0x00,
  ADDC = 0x01,
  SUB = 0x02,
  SUBB = 0x03,
  AND = 0x04,
  OR = 0x05,
  XOR = 0x06,
  SPECIAL = 0x07,

  // Shift instructions are treated as SPECIAL when encoding the machine
  // instruction, but kept distinct until lowering. The constant values are
  // chosen to ease lowering.
  SHL = 0x17,
  SRL = 0x27,
  SRA = 0x37,

  // Indicates an unknown/unsupported operator
  UNKNOWN = 0xFF,
};

// Bits indicating post- and pre-operators should be tested and set using Is*
// and Make* utility functions
const int PCPU_PRE_OP = 0x40;
const int PCPU_POST_OP = 0x80;

inline static unsigned encodePCPUAluCode(unsigned AluOp) {
  unsigned const OP_ENCODING_MASK = 0x07;
  return AluOp & OP_ENCODING_MASK;
}

inline static unsigned getAluOp(unsigned AluOp) {
  unsigned const ALU_MASK = 0x3F;
  return AluOp & ALU_MASK;
}

inline static bool isPreOp(unsigned AluOp) { return AluOp & PCPU_PRE_OP; }

inline static bool isPostOp(unsigned AluOp) { return AluOp & PCPU_POST_OP; }

inline static unsigned makePreOp(unsigned AluOp) {
  assert(!isPostOp(AluOp) && "Operator can't be a post- and pre-op");
  return AluOp | PCPU_PRE_OP;
}

inline static unsigned makePostOp(unsigned AluOp) {
  assert(!isPreOp(AluOp) && "Operator can't be a post- and pre-op");
  return AluOp | PCPU_POST_OP;
}

inline static bool modifiesOp(unsigned AluOp) {
  return isPreOp(AluOp) || isPostOp(AluOp);
}

inline static const char *PCPUAluCodeToString(unsigned AluOp) {
  switch (getAluOp(AluOp)) {
  case ADD:
    return "add";
  case ADDC:
    return "addc";
  case SUB:
    return "sub";
  case SUBB:
    return "subb";
  case AND:
    return "and";
  case OR:
    return "or";
  case XOR:
    return "xor";
  case SHL:
    return "sh";
  case SRL:
    return "sh";
  case SRA:
    return "sha";
  default:
    llvm_unreachable("Invalid ALU code.");
  }
}

inline static AluCode stringToPCPUAluCode(StringRef S) {
  return StringSwitch<AluCode>(S)
      .Case("add", ADD)
      .Case("addc", ADDC)
      .Case("sub", SUB)
      .Case("subb", SUBB)
      .Case("and", AND)
      .Case("or", OR)
      .Case("xor", XOR)
      .Case("sh", SHL)
      .Case("srl", SRL)
      .Case("sha", SRA)
      .Default(UNKNOWN);
}

inline static AluCode isdToPCPUAluCode(ISD::NodeType Node_type) {
  switch (Node_type) {
  case ISD::ADD:
    return AluCode::ADD;
  case ISD::ADDE:
    return AluCode::ADDC;
  case ISD::SUB:
    return AluCode::SUB;
  case ISD::SUBE:
    return AluCode::SUBB;
  case ISD::AND:
    return AluCode::AND;
  case ISD::OR:
    return AluCode::OR;
  case ISD::XOR:
    return AluCode::XOR;
  case ISD::SHL:
    return AluCode::SHL;
  case ISD::SRL:
    return AluCode::SRL;
  case ISD::SRA:
    return AluCode::SRA;
  default:
    return AluCode::UNKNOWN;
  }
}
} // namespace LPAC
} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUALUCODE_H
