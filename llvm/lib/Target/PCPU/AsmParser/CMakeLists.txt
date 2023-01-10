include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_llvm_component_library(LLVMPCPUAsmParser
  PCPUAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  Support
  PCPUDesc
  PCPUInfo

  ADD_TO_COMPONENT
  PCPU
  )

add_dependencies( LLVMPCPUAsmParser PCPUCommonTableGen )
