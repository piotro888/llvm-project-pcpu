add_llvm_component_library(LLVMPCPUInfo
  PCPUTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  PCPU
  )
