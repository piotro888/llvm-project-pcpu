//===- PCPUSubtarget.cpp - PCPU Subtarget Information -----------*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the PCPU specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "PCPUSubtarget.h"

#include "PCPU.h"

#define DEBUG_TYPE "PCPU-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "PCPUGenSubtargetInfo.inc"

using namespace llvm;

void PCPUSubtarget::initSubtargetFeatures(StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";

  ParseSubtargetFeatures(CPUName, /*TuneCPU*/ CPUName, FS);
}

PCPUSubtarget &PCPUSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                                StringRef FS) {
  initSubtargetFeatures(CPU, FS);
  return *this;
}

PCPUSubtarget::PCPUSubtarget(const Triple &TargetTriple, StringRef Cpu,
                               StringRef FeatureString, const TargetMachine &TM,
                               const TargetOptions & /*Options*/,
                               CodeModel::Model /*CodeModel*/,
                               CodeGenOpt::Level /*OptLevel*/)
    : PCPUGenSubtargetInfo(TargetTriple, Cpu, /*TuneCPU*/ Cpu, FeatureString),
      FrameLowering(initializeSubtargetDependencies(Cpu, FeatureString)),
      TLInfo(TM, *this) {}
