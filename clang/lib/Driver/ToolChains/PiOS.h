//===--- PiOS.h - PiOS ToolChain Implementations ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_PIOS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_PIOS_H

#include "Gnu.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace tools {

namespace pios {
class LLVM_LIBRARY_VISIBILITY Assembler : public Tool {
public:
  Assembler(const ToolChain &TC)
      : Tool("pios::Assembler", "assembler", TC) {}

  bool hasIntegratedCPP() const override { return false; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

class LLVM_LIBRARY_VISIBILITY Linker : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("pios::Linker", "linker", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};
} // end namespace pios
} // end namespace tools

namespace toolchains {

class LLVM_LIBRARY_VISIBILITY  PiOS : public Generic_ELF {
public:
  PiOS(const Driver &D, const llvm::Triple &Triple,
          const llvm::opt::ArgList &Args);

  bool HasNativeLLVMSupport() const override;

  bool IsMathErrnoDefault() const override { return false; }
  bool IsObjCNonFragileABIDefault() const override { return true; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }

  RuntimeLibType GetDefaultRuntimeLibType() const override {
    return ToolChain::RLT_CompilerRT;
  }
  CXXStdlibType GetDefaultCXXStdlibType() const override {
    return ToolChain::CST_Libcxx;
  }

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;

  void addLibCxxIncludePaths(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args) const override;
  void AddCXXStdlibLibArgs(const llvm::opt::ArgList &Args,
                           llvm::opt::ArgStringList &CmdArgs) const override;

  std::string getCompilerRT(const llvm::opt::ArgList &Args, StringRef Component,
                            FileType Type = ToolChain::FT_Static) const override;

  void addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                           llvm::opt::ArgStringList &CC1Args,
                           Action::OffloadKind DeviceOffloadKind) const override;
                            
  unsigned GetDefaultDwarfVersion() const override { return 2; }

  const char *getDefaultLinker() const override { return "ld.lld"; }

protected:
  Tool *buildAssembler() const override;
  Tool *buildLinker() const override;
};

} // end namespace toolchains
} // end namespace driver
} // end namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_PIOS_H
