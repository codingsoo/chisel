#include "LocalReduction.h"

#include <spdlog/spdlog.h>

#include "FileManager.h"
#include "OptionManager.h"
#include "Profiler.h"
#include "StringUtils.h"

using BinaryOperator = clang::BinaryOperator;
using BreakStmt = clang::BreakStmt;
using CallExpr = clang::CallExpr;
using ContinueStmt = clang::ContinueStmt;
using CompoundStmt = clang::CompoundStmt;
using DeclGroupRef = clang::DeclGroupRef;
using DeclStmt = clang::DeclStmt;
using FunctionDecl = clang::FunctionDecl;
using GotoStmt = clang::GotoStmt;
using IfStmt = clang::IfStmt;
using LabelStmt = clang::LabelStmt;
using ReturnStmt = clang::ReturnStmt;
using SourceRange = clang::SourceRange;
using SourceLocation = clang::SourceLocation;
using Stmt = clang::Stmt;
using UnaryOperator = clang::UnaryOperator;
using WhileStmt = clang::WhileStmt;

void LocalReduction::Initialize(clang::ASTContext &Ctx) {
  Reduction::Initialize(Ctx);
  CollectionVisitor = new LocalElementCollectionVisitor(this);
}

bool LocalReduction::HandleTopLevelDecl(DeclGroupRef D) {
  for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
    CollectionVisitor->TraverseDecl(*I);
  }
  return true;
}

void LocalReduction::HandleTranslationUnit(clang::ASTContext &Ctx) {
  for (auto const &body : FunctionBodies) {
    Queue.push(body);
    while (!Queue.empty()) {
      Stmt *S = Queue.front();
      Queue.pop();
      doHierarchicalDeltaDebugging(S);
    }
  }
}

DDElement LocalReduction::CastElement(Stmt *S) { return S; }

bool LocalReduction::callOracle() {
  Profiler::GetInstance()->incrementLocalReductionCounter();
  std::string TempFileName =
      FileManager::GetInstance()->getTempFileName("local");

  if (Reduction::callOracle()) {
    FileManager::GetInstance()->updateBest();
    Profiler::GetInstance()->incrementSuccessfulLocalReductionCounter();
    if (OptionManager::SaveTemp)
      writeToFile(TempFileName + ".success.c");
    return true;
  } else {
    if (OptionManager::SaveTemp)
      writeToFile(TempFileName + ".fail.c");
    return false;
  }
}

bool LocalReduction::test(std::vector<DDElement> &toBeRemoved) {
  const clang::SourceManager *SM = &Context->getSourceManager();
  SourceLocation TotalStart, TotalEnd;
  TotalStart = toBeRemoved.front().get<Stmt *>()->getSourceRange().getBegin();
  Stmt *Last = toBeRemoved.back().get<Stmt *>();

  if (CompoundStmt *CS = llvm::dyn_cast<CompoundStmt>(Last)) {
    TotalEnd = CS->getRBracLoc().getLocWithOffset(1);
  } else if (IfStmt *IS = llvm::dyn_cast<IfStmt>(Last)) {
    TotalEnd = Last->getSourceRange().getEnd().getLocWithOffset(1);
  } else if (WhileStmt *WS = llvm::dyn_cast<WhileStmt>(Last)) {
    TotalEnd = Last->getSourceRange().getEnd().getLocWithOffset(1);
  } else if (LabelStmt *LS = llvm::dyn_cast<LabelStmt>(Last)) {
    auto subStmt = LS->getSubStmt();
    if (CompoundStmt *LS_CS = llvm::dyn_cast<CompoundStmt>(subStmt)) {
      TotalEnd = LS_CS->getRBracLoc().getLocWithOffset(1);
    } else {
      TotalEnd = getEndLocationUntil(subStmt->getSourceRange(), ';')
                     .getLocWithOffset(1);
    }
  } else if (BinaryOperator *BO = llvm::dyn_cast<BinaryOperator>(Last)) {
    TotalEnd = getEndLocationAfter(Last->getSourceRange(), ';');
  } else if (ReturnStmt *RS = llvm::dyn_cast<ReturnStmt>(Last)) {
    TotalEnd = getEndLocationAfter(RS->getSourceRange(), ';');
  } else if (GotoStmt *GS = llvm::dyn_cast<GotoStmt>(Last)) {
    TotalEnd =
        getEndLocationUntil(Last->getSourceRange(), ';').getLocWithOffset(1);
  } else if (BreakStmt *BS = llvm::dyn_cast<BreakStmt>(Last)) {
    TotalEnd =
        getEndLocationUntil(Last->getSourceRange(), ';').getLocWithOffset(1);
  } else if (ContinueStmt *CS = llvm::dyn_cast<ContinueStmt>(Last)) {
    TotalEnd =
        getEndLocationUntil(Last->getSourceRange(), ';').getLocWithOffset(1);
  } else if (DeclStmt *DS = llvm::dyn_cast<DeclStmt>(Last)) {
    TotalEnd = getEndLocationAfter(Last->getSourceRange(), ';');
  } else if (CallExpr *CE = llvm::dyn_cast<CallExpr>(Last)) {
    TotalEnd =
        getEndLocationUntil(Last->getSourceRange(), ';').getLocWithOffset(1);
  } else if (UnaryOperator *UO = llvm::dyn_cast<UnaryOperator>(Last)) {
    TotalEnd =
        getEndLocationUntil(Last->getSourceRange(), ';').getLocWithOffset(1);
  } else {
    return false;
  }

  if (TotalEnd.isInvalid() || TotalStart.isInvalid())
    return false;

  SourceRange Range(TotalStart, TotalEnd);
  std::string Revert = getSourceText(Range);
  TheRewriter.ReplaceText(Range, StringUtils::placeholder(Revert));
  writeToFile(OptionManager::InputFile);

  if (callOracle()) {
    return true;
  } else {
    TheRewriter.ReplaceText(Range, Revert);
    writeToFile(OptionManager::InputFile);
    return false;
  }
}

std::vector<DDElementVector>
LocalReduction::refineSubsets(std::vector<DDElementVector> &Subsets) {
  return Subsets;
}

void LocalReduction::doHierarchicalDeltaDebugging(Stmt *S) {
  if (S == NULL)
    return;

  if (IfStmt *IS = llvm::dyn_cast<IfStmt>(S)) {
    spdlog::get("Logger")->debug("HDD IF");
    reduceIf(IS);
  } else if (WhileStmt *WS = llvm::dyn_cast<WhileStmt>(S)) {
    spdlog::get("Logger")->debug("HDD WHILE");
    reduceWhile(WS);
  } else if (CompoundStmt *CS = llvm::dyn_cast<CompoundStmt>(S)) {
    spdlog::get("Logger")->debug("HDD Compound");
    reduceCompound(CS);
  } else if (LabelStmt *LS = llvm::dyn_cast<LabelStmt>(S)) {
    spdlog::get("Logger")->debug("HDD Label");
    reduceLabel(LS);
  } else {
    return;
    // TODO add other cases: For, Do, Switch...
  }
}

// kihong: is this necessary?
std::vector<Stmt *> LocalReduction::getImmediateChildren(Stmt *S) {
  std::vector<Stmt *> children;
  for (auto S : S->children()) {
    children.emplace_back(S);
  }
  return children;
}

std::vector<Stmt *> LocalReduction::getBodyStatements(CompoundStmt *CS) {
  std::vector<Stmt *> stmts;
  for (auto S : CS->body()) {
    stmts.emplace_back(S);
  }
  return stmts;
}

void LocalReduction::reduceIf(IfStmt *IS) {
  // remove else branch
  SourceLocation beginIf = IS->getSourceRange().getBegin();
  SourceLocation endIf = IS->getSourceRange().getEnd().getLocWithOffset(1);
  SourceLocation endCond =
      IS->getThen()->getSourceRange().getBegin().getLocWithOffset(-1);
  SourceLocation endThen = IS->getThen()->getSourceRange().getEnd();
  SourceLocation elseLoc;

  Stmt *Then = IS->getThen();
  Stmt *Else = IS->getElse();
  Stmt *ElseAsCompoundStmt;
  if (Else) {
    elseLoc = IS->getElseLoc();
    // kihong: ??
    ElseAsCompoundStmt = getImmediateChildren(IS).back();
    if (elseLoc.isInvalid())
      return;
  }

  if (beginIf.isInvalid() || endIf.isInvalid() || endCond.isInvalid() ||
      endThen.isInvalid())
    return;

  std::string revertIf = getSourceText(SourceRange(beginIf, endIf));

  if (Else) { // then, else
    // remove else branch
    std::string ifAndCond = getSourceText(SourceRange(beginIf, endCond));
    TheRewriter.ReplaceText(SourceRange(beginIf, endCond),
                            StringUtils::placeholder(ifAndCond));
    std::string elsePart = getSourceText(SourceRange(elseLoc, endIf));
    TheRewriter.ReplaceText(SourceRange(elseLoc, endIf),
                            StringUtils::placeholder(elsePart));
    writeToFile(OptionManager::InputFile);
    if (callOracle()) { // successfully remove else branch
      Queue.push(Then);
    } else { // revert else branch removal
      TheRewriter.ReplaceText(SourceRange(beginIf, endIf), revertIf);
      writeToFile(OptionManager::InputFile);
      // remove then branch
      std::string ifAndThenAndElseWord =
          getSourceText(SourceRange(beginIf, elseLoc.getLocWithOffset(4)));
      TheRewriter.ReplaceText(SourceRange(beginIf, elseLoc.getLocWithOffset(4)),
                              StringUtils::placeholder(ifAndThenAndElseWord));
      writeToFile(OptionManager::InputFile);
      if (callOracle()) { // successfully remove then branch
        Queue.push(ElseAsCompoundStmt);
      } else { // revert then branch removal
        TheRewriter.ReplaceText(SourceRange(beginIf, endIf), revertIf);
        writeToFile(OptionManager::InputFile);
        Queue.push(Then);
        Queue.push(ElseAsCompoundStmt);
      }
    }
  } else { // then
    // remove condition
    std::string ifAndCond = getSourceText(SourceRange(beginIf, endCond));
    TheRewriter.ReplaceText(SourceRange(beginIf, endCond),
                            StringUtils::placeholder(ifAndCond));
    writeToFile(OptionManager::InputFile);
    if (callOracle()) {
      Queue.push(Then);
    } else {
      TheRewriter.ReplaceText(SourceRange(beginIf, endIf), revertIf);
      writeToFile(OptionManager::InputFile);
      Queue.push(Then);
    }
  }
}

void LocalReduction::reduceWhile(WhileStmt *WS) {
  auto body = WS->getBody();
  SourceLocation beginWhile = WS->getSourceRange().getBegin();
  SourceLocation endWhile = WS->getSourceRange().getEnd().getLocWithOffset(1);
  SourceLocation endCond =
      body->getSourceRange().getBegin().getLocWithOffset(-1);

  std::string revertWhile = getSourceText(SourceRange(beginWhile, endWhile));

  std::string whileAndCond = getSourceText(SourceRange(beginWhile, endCond));
  TheRewriter.ReplaceText(SourceRange(beginWhile, endCond),
                          StringUtils::placeholder(whileAndCond));
  writeToFile(OptionManager::InputFile);
  if (callOracle()) {
    Queue.push(body);
  } else {
    // revert
    TheRewriter.ReplaceText(SourceRange(beginWhile, endWhile), revertWhile);
    writeToFile(OptionManager::InputFile);
    Queue.push(body);
  }
}

void LocalReduction::reduceCompound(CompoundStmt *CS) {
  auto stmts = getBodyStatements(CS);
  for (auto stmt : stmts)
    Queue.push(stmt);
  std::vector<DDElement> elements;
  elements.resize(stmts.size());
  std::transform(stmts.begin(), stmts.end(), elements.begin(), CastElement);
  doDeltaDebugging(elements);
}

void LocalReduction::reduceLabel(LabelStmt *LS) {
  if (CompoundStmt *CS = llvm::dyn_cast<CompoundStmt>(LS->getSubStmt())) {
    Queue.push(CS);
  }
}

bool LocalElementCollectionVisitor::VisitFunctionDecl(FunctionDecl *FD) {
  spdlog::get("Logger")->debug("Visit Function Decl: {}",
                               FD->getNameInfo().getAsString());
  if (FD->isThisDeclarationADefinition()) {
    Consumer->FunctionBodies.emplace_back(FD->getBody());
  }
  return true;
}
