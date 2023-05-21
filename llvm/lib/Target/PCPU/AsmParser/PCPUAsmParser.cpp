//===- PCPUAsmParser.cpp - Parse PCPU assembly to MCInst instructions -===//
//
//                     The LLVM Compiler Infrastructure
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/PCPUMCTargetDesc.h"
#include "TargetInfo/PCPUTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

#define DEBUG_TYPE "PCPU-asm-parser"

struct PCPUOperand;

class PCPUAsmParser : public MCTargetAsmParser {

  SMLoc getLoc() const { return getParser().getTok().getLoc(); }

  // Override MCTargetAsmParser.
  bool ParseDirective(AsmToken DirectiveID) override;
  bool parseRegister(MCRegister &RegNo,
                     SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "PCPUGenAsmMatcher.inc"

  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseRegister(OperandVector &Operands,
                                     bool AllowParens = false, bool SR = false);
  OperandMatchResultTy parseOperandWithModifier(OperandVector &Operands);
  bool parseOperand(OperandVector &Operands, StringRef Mnemonic,
                    bool SR = false);
  bool ParseInstructionWithSR(ParseInstructionInfo &Info, StringRef Name,
                              SMLoc NameLoc, OperandVector &Operands);
  OperandMatchResultTy tryParseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override {
    return MatchOperand_NoMatch;
  }
  OperandMatchResultTy parsePCRelTarget(OperandVector &Operands);

public:
  enum PCPUMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "PCPUGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

  PCPUAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

// Return true if Expr is in the range [MinValue, MaxValue].
static bool inRange(const MCExpr *Expr, int64_t MinValue, int64_t MaxValue) {
  if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    int64_t Value = CE->getValue();
    return Value >= MinValue && Value <= MaxValue;
  }
  return false;
}

struct PCPUOperand : public MCParsedAsmOperand {

  enum KindTy {
    Token,
    Register,
    Immediate,
  } Kind;

  struct RegOp {
    unsigned RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  SMLoc StartLoc, EndLoc;
  union {
    StringRef Tok;
    RegOp Reg;
    ImmOp Imm;
  };

  PCPUOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
  PCPUOperand(const PCPUOperand &o) : MCParsedAsmOperand() {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
    case Register:
      Reg = o.Reg;
      break;
    case Immediate:
      Imm = o.Imm;
      break;
    case Token:
      Tok = o.Tok;
      break;
    }
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }

  bool isImm(int64_t MinValue, int64_t MaxValue) const {
    return Kind == Immediate && inRange(getImm(), MinValue, MaxValue);
  }

  bool isImm8() const { return isImm(-128, 127); }


  bool isOffset8m8() const { return isImm(0, 255); }

  bool isOffset8m16() const {
    return isImm(0, 510) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x1) == 0);
  }

  bool isOffset8m32() const {
    return isImm(0, 1020) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x3) == 0);
  }
  bool isImm16_31() const { return isImm(16, 31); }

  bool isImm1_16() const { return isImm(1, 16); }

  /// getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }

  unsigned getReg() const override {
    assert(Kind == Register && "Invalid type access!");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate && "Invalid type access!");
    return Imm.Val;
  }

  StringRef getToken() const {
    assert(Kind == Token && "Invalid type access!");
    return Tok;
  }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case Immediate:
      OS << *getImm();
      break;
    case Register:
      OS << "<register x";
      OS << getReg() << ">";
      break;
    case Token:
      OS << "'" << getToken() << "'";
      break;
    }
  }

  static std::unique_ptr<PCPUOperand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<PCPUOperand>(Token);
    Op->Tok = Str;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<PCPUOperand> createReg(unsigned RegNo, SMLoc S,
                                                  SMLoc E) {
    auto Op = std::make_unique<PCPUOperand>(Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<PCPUOperand> createImm(const MCExpr *Val, SMLoc S,
                                                  SMLoc E) {
    auto Op = std::make_unique<PCPUOperand>(Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    assert(Expr && "Expr shouldn't be null!");
    int64_t Imm = 0;
    bool IsConstant = false;

    if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
      IsConstant = true;
      Imm = CE->getValue();
    }

    if (IsConstant)
      Inst.addOperand(MCOperand::createImm(Imm));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    addExpr(Inst, getImm());
  }
};

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "PCPUGenAsmMatcher.inc"

unsigned PCPUAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                     unsigned Kind) {
  return Match_InvalidOperand;
}

static SMLoc RefineErrorLoc(const SMLoc Loc, const OperandVector &Operands,
                            uint64_t ErrorInfo) {
  if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size()) {
    SMLoc ErrorLoc = Operands[ErrorInfo]->getStartLoc();
    if (ErrorLoc == SMLoc())
      return Loc;
    return ErrorLoc;
  }
  return Loc;
}

bool PCPUAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                              OperandVector &Operands,
                                              MCStreamer &Out,
                                              uint64_t &ErrorInfo,
                                              bool MatchingInlineAsm) {
  MCInst Inst;
  auto Result =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm);

  switch (Result) {
  default:
    break;
  case Match_Success:
    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "instruction use requires an option to be enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");

      ErrorLoc = ((PCPUOperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  // case Match_InvalidImm16:
  //   return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
  //                "expected immediate in range [-32768, 32512], first 8 bits "
  //                "should be zero");
  }

  report_fatal_error("Unknown match type detected!");
}

OperandMatchResultTy
PCPUAsmParser::parsePCRelTarget(OperandVector &Operands) {
  MCAsmParser &Parser = getParser();
  LLVM_DEBUG(dbgs() << "parsePCRelTarget\n");

  SMLoc S = getLexer().getLoc();

  // Expressions are acceptable
  const MCExpr *Expr = nullptr;
  if (Parser.parseExpression(Expr)) {
    // We have no way of knowing if a symbol was consumed so we must ParseFail
    return MatchOperand_ParseFail;
  }

  // Currently not support constants
  if (Expr->getKind() == MCExpr::ExprKind::Constant) {
    Error(getLoc(), "unknown operand");
    return MatchOperand_ParseFail;
  }

  Operands.push_back(PCPUOperand::createImm(Expr, S, getLexer().getLoc()));
  return MatchOperand_Success;
}

bool PCPUAsmParser::parseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                    SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  RegNo = 0;
  StringRef Name = getLexer().getTok().getIdentifier();

  if (!MatchRegisterName(Name)) {
    getParser().Lex(); // Eat identifier token.
    return false;
  }

  return Error(StartLoc, "invalid register name");
}

OperandMatchResultTy PCPUAsmParser::parseRegister(OperandVector &Operands,
                                                    bool AllowParens, bool SR) {
  SMLoc FirstS = getLoc();
  bool HadParens = false;
  AsmToken Buf[2];
  StringRef RegName;

  // If this a parenthesised register name is allowed, parse it atomically
  if (AllowParens && getLexer().is(AsmToken::LParen)) {
    size_t ReadCount = getLexer().peekTokens(Buf);
    if (ReadCount == 2 && Buf[1].getKind() == AsmToken::RParen) {
      if ((Buf[0].getKind() == AsmToken::Integer) && (!SR))
        return MatchOperand_NoMatch;
      HadParens = true;
      getParser().Lex(); // Eat '('
    }
  }

  unsigned RegNo = 0;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::Integer:
    if (!SR)
      return MatchOperand_NoMatch;
    RegName = StringRef(std::to_string(getLexer().getTok().getIntVal()));
    RegNo = MatchRegisterName(RegName);
    break;
  case AsmToken::Identifier:
    RegName = getLexer().getTok().getIdentifier();
    RegNo = MatchRegisterName(RegName);
    break;
  }

  if (RegNo == 0) {
    if (HadParens)
      getLexer().UnLex(Buf[0]);
    return MatchOperand_NoMatch;
  }
  if (HadParens)
    Operands.push_back(PCPUOperand::createToken("(", FirstS));
  SMLoc S = getLoc();
  SMLoc E = getParser().getTok().getEndLoc();
  getLexer().Lex();
  Operands.push_back(PCPUOperand::createReg(RegNo, S, E));

  if (HadParens) {
    getParser().Lex(); // Eat ')'
    Operands.push_back(PCPUOperand::createToken(")", getLoc()));
  }

  return MatchOperand_Success;
}

OperandMatchResultTy PCPUAsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E;
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String:
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;
    break;
  case AsmToken::Identifier: {
    StringRef Identifier;
    if (getParser().parseIdentifier(Identifier))
      return MatchOperand_ParseFail;

    MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);
    Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());
    break;
  }
  case AsmToken::Percent:
    return parseOperandWithModifier(Operands);
  }

  E = SMLoc::getFromPointer(S.getPointer() - 1);
  Operands.push_back(PCPUOperand::createImm(Res, S, E));
  return MatchOperand_Success;
}

OperandMatchResultTy
PCPUAsmParser::parseOperandWithModifier(OperandVector &Operands) {
  return MatchOperand_ParseFail;
}

/// Looks at a token type and creates the relevant operand
/// from this information, adding to Operands.
/// If operand was parsed, returns false, else true.
bool PCPUAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic,
                                   bool SR) {
  /*
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);
  if (ResTy == MatchOperand_Success)
    return false;

  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_ParseFail)
    return true;
  */
 
  // Attempt to parse token as register
  if (parseRegister(Operands, true, SR) == MatchOperand_Success)
    return false;

  // Attempt to parse token as an immediate
  if (parseImmediate(Operands) == MatchOperand_Success) {
    return false;
  }

  // Finally we have exhausted all options and must declare defeat.
  Error(getLoc(), "unknown operand");
  return true;
}

bool PCPUAsmParser::ParseInstructionWithSR(ParseInstructionInfo &Info,
                                             StringRef Name, SMLoc NameLoc,
                                             OperandVector &Operands) {
  if ((Name.startswith("wsr.") || Name.startswith("rsr.") ||
       Name.startswith("xsr.")) &&
      (Name.size() > 4)) {
    // Parse case when instruction name is concatenated with SR register
    // name, like "wsr.sar a1"

    // First operand is token for instruction
    Operands.push_back(PCPUOperand::createToken(Name.take_front(3), NameLoc));

    StringRef RegName = Name.drop_front(4);
    unsigned RegNo = MatchRegisterName(RegName);

    if (RegNo == 0) {
      Error(NameLoc, "invalid register name");
      return true;
    }

    // Parse operand
    if (parseOperand(Operands, Name))
      return true;

    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    Operands.push_back(PCPUOperand::createReg(RegNo, S, E));
  } else {
    // First operand is token for instruction
    Operands.push_back(PCPUOperand::createToken(Name, NameLoc));

    // Parse first operand
    if (parseOperand(Operands, Name))
      return true;

    if (!getLexer().is(AsmToken::Comma)) {
      SMLoc Loc = getLexer().getLoc();
      getParser().eatToEndOfStatement();
      return Error(Loc, "unexpected token");
    }

    getLexer().Lex();

    // Parse second operand
    if (parseOperand(Operands, Name, true))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool PCPUAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                       StringRef Name, SMLoc NameLoc,
                                       OperandVector &Operands) {
  if (Name.startswith("wsr") || Name.startswith("rsr") ||
      Name.startswith("xsr")) {
    return ParseInstructionWithSR(Info, Name, NameLoc, Operands);
  }

  // First operand is token for instruction
  Operands.push_back(PCPUOperand::createToken(Name, NameLoc));

  // If there are no more operands, then finish
  if (getLexer().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (parseOperand(Operands, Name))
    return true;

  // Parse until end of statement, consuming commas between operands
  while (getLexer().is(AsmToken::Comma)) {
    // Consume comma token
    getLexer().Lex();

    // Parse next operand
    if (parseOperand(Operands, Name))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool PCPUAsmParser::ParseDirective(AsmToken DirectiveID) { return true; }

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePCPUAsmParser() {
  RegisterMCAsmParser<PCPUAsmParser> X(getThePCPUTarget());
}