//===- PCPU.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class PCPU final : public TargetInfo {
public:
  PCPU();
  uint32_t calcEFlags() const override;
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

PCPU::PCPU() {
  defaultImageBase = 0x0;
}

RelExpr PCPU::getRelExpr(RelType type, const Symbol &s,
                        const uint8_t *loc) const {
  switch (type) {
  case R_PCPU_PC:
  case R_PCPU_8:
  case R_PCPU_16:
  case R_PCPU_32:
  case R_PCPU_64: //??
     return R_ABS;
  default:
    error(getErrorLocation(loc) + "unknown relocation (" + Twine(type) +
          ") against symbol " + toString(s));
    return R_NONE;
  }
}

void PCPU::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_PCPU_PC:
    // PC field is located at offset 2 in little-endian instrucion encoding
    // PC value in PCPU is not instruction address, but index - address divided by size (4)
    write16le(loc+2, (val>>2));
    break;
  case R_PCPU_8:
    *loc = val;
    break;
  case R_PCPU_16:
    write16le(loc, val);
    break;
  case R_PCPU_32:
    write32le(loc, val);
    break;
  case R_PCPU_64:
    write64le(loc, val);
    break;
  default:
    llvm_unreachable("unknown relocation");
  }
}

TargetInfo *elf::getPCPUTargetInfo() {
  static PCPU target;
  return &target;
}

static uint32_t getEFlags(InputFile *file) {
  return cast<ObjFile<ELF32LE>>(file)->getObj().getHeader().e_flags;
}

uint32_t PCPU::calcEFlags() const {
  assert(!ctx.objectFiles.empty());

  uint32_t flags = getEFlags(ctx.objectFiles[0]);

  return flags;
}
