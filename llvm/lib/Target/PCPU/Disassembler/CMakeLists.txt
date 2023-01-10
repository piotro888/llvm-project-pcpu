add_llvm_component_library(LLVMPCPUDisassembler
  PCPUDisassembler.cpp

  LINK_COMPONENTS
  PCPUDesc
  PCPUInfo
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  PCPU
  )
