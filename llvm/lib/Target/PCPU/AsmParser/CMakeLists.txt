add_llvm_component_library(LLVMPCPUAsmParser
  PCPUAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  PCPUDesc
  PCPUInfo
  Support

  ADD_TO_COMPONENT
  PCPU
)
