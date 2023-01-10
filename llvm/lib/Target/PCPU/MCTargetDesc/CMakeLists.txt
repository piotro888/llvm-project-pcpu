add_llvm_component_library(LLVMPCPUDesc
  PCPUAsmBackend.cpp
  PCPUELFObjectWriter.cpp
  PCPUInstPrinter.cpp
  PCPUMCAsmInfo.cpp
  PCPUMCCodeEmitter.cpp
  PCPUMCExpr.cpp
  PCPUMCTargetDesc.cpp

  LINK_COMPONENTS
  PCPUInfo
  MC
  MCDisassembler
  Support
  TargetParser

  ADD_TO_COMPONENT
  PCPU
)
