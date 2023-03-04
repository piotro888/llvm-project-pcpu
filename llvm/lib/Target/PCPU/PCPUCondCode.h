// The encoding used for conditional codes used in BR instructions

#ifndef LLVM_LIB_TARGET_PCPU_PCPUCONDCODE_H
#define LLVM_LIB_TARGET_PCPU_PCPUCONDCODE_H

#include "llvm/ADT/StringSwitch.h"

namespace llvm {
namespace LPCC {
enum CondCode {
  ICC_T = 0,
  ICC_CA = 1,
  ICC_EQ = 2,
  ICC_LT = 3,
  ICC_GT = 4,
  ICC_LE = 5,
  ICC_GE = 6,
  ICC_NE = 7,
  ICC_OVF = 8,
  ICC_GTU = 9,
  ICC_GEU = 10,
  ICC_LEU = 11,
  UNKNOWN
};

inline static StringRef PCPUCondCodeToString(LPCC::CondCode CC) {
  switch (CC) {
  case LPCC::ICC_T:
    return "mp"; //jmp - unconditional
  case LPCC::ICC_CA:
    return "ca";
  case LPCC::ICC_EQ:
    return "eq";
  case LPCC::ICC_LT:
    return "lt";
  case LPCC::ICC_GT:
    return "gt";
  case LPCC::ICC_LE:
    return "le";
  case LPCC::ICC_GE:
    return "ge";
  case LPCC::ICC_NE:
    return "ne";
  case LPCC::ICC_OVF:
    return "ov";
  case LPCC::ICC_GTU:
    return "gtu";
  case LPCC::ICC_LEU:
    return "leu";
  case LPCC::ICC_GEU:
    return "geu";
  default:
    llvm_unreachable("Invalid cond code");
  }
}

inline static CondCode suffixToPCPUCondCode(StringRef S) {
  return StringSwitch<CondCode>(S)
      .EndsWith("ule", LPCC::ICC_LEU)
      .EndsWith("geu", LPCC::ICC_GEU)
      .EndsWith("gtu", LPCC::ICC_GTU)
      .EndsWith("ov", LPCC::ICC_OVF)
      .EndsWith("ne", LPCC::ICC_NE)
      .EndsWith("eq", LPCC::ICC_EQ)
      .EndsWith("ge", LPCC::ICC_GE)
      .EndsWith("lt", LPCC::ICC_LT)
      .EndsWith("gt", LPCC::ICC_GT)
      .EndsWith("le", LPCC::ICC_LE)
      .EndsWith("ca", LPCC::ICC_CA)
      .EndsWith("mp", LPCC::ICC_T)
      .Default(LPCC::UNKNOWN);
}
} // namespace LPCC
} // namespace llvm

#endif // LLVM_LIB_TARGET_PCPU_PCPUCONDCODE_H
