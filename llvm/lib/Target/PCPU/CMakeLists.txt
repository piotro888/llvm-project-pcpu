add_llvm_component_group(PCPU)

set(LLVM_TARGET_DEFINITIONS PCPU.td)

tablegen(LLVM PCPUGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM PCPUGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM PCPUGenCallingConv.inc -gen-callingconv)
tablegen(LLVM PCPUGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM PCPUGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM PCPUGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM PCPUGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM PCPUGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM PCPUGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM PCPUGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(PCPUCommonTableGen)

add_llvm_target(PCPUCodeGen
  PCPUAsmPrinter.cpp
  PCPUFrameLowering.cpp
  PCPUInstrInfo.cpp
  PCPUISelDAGToDAG.cpp
  PCPUISelLowering.cpp
  PCPUMachineFunctionInfo.cpp
  PCPUMCInstLower.cpp
  PCPURegisterInfo.cpp
  PCPUSelectionDAGInfo.cpp
  PCPUSubtarget.cpp
  PCPUTargetMachine.cpp
  PCPUTargetObjectFile.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  PCPUDesc
  PCPUInfo
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils

  ADD_TO_COMPONENT
  PCPU
)

add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
