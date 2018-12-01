#ifndef LOCAL_REDUCTION_H
#define LOCAL_REDUCTION_H

#include <queue>
#include <vector>

#include "clang/AST/RecursiveASTVisitor.h"

#include "Reduction.h"

class LocalElementCollectionVisitor;

class LocalReduction : public Reduction {
  friend class LocalElementCollectionVisitor;

public:
  LocalReduction() : CollectionVisitor(NULL) {}
  ~LocalReduction() { delete CollectionVisitor; }

private:
  void Initialize(clang::ASTContext &Ctx);
  bool HandleTopLevelDecl(clang::DeclGroupRef D);
  void HandleTranslationUnit(clang::ASTContext &Ctx);

  static DDElement CastElement(clang::Stmt *S);

  bool callOracle();
  bool test(std::vector<DDElement> &ToBeRemoved);
  std::vector<DDElementVector>
  refineChunks(std::vector<DDElementVector> &Chunks);

  void doHierarchicalDeltaDebugging(clang::Stmt *S);
  void reduceIf(clang::IfStmt *IS);
  void reduceWhile(clang::WhileStmt *WS);
  void reduceCompound(clang::CompoundStmt *CS);
  void reduceLabel(clang::LabelStmt *LS);
  DDElementVector getAllChildren(clang::Stmt *S);
  int countReturnStmts(DDElementVector &Elements);
  bool noReturn(DDElementVector &FunctionStmts,
                DDElementVector &AllRemovedStmts);
  bool danglingLabel(DDElementVector &FunctionStmts,
                     DDElementVector &AllRemovedStmts);
  bool brokenDependency(DDElementVector &Chunk);

  std::vector<clang::Stmt *> getBodyStatements(clang::CompoundStmt *CS);

  clang::SourceLocation getEndLocation(clang::SourceLocation Loc);

  std::vector<clang::Decl *> Functions;
  std::queue<clang::Stmt *> Queue;
  std::map<clang::Decl *, std::vector<clang::DeclRefExpr *>> UseInfo;

  LocalElementCollectionVisitor *CollectionVisitor;
  clang::FunctionDecl *CurrentFunction;
};

class LocalElementCollectionVisitor
    : public clang::RecursiveASTVisitor<LocalElementCollectionVisitor> {
public:
  LocalElementCollectionVisitor(LocalReduction *R) : Consumer(R) {}

  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitDeclRefExpr(clang::DeclRefExpr *DRE);

private:
  LocalReduction *Consumer;
};

#endif // LOCAL_REDUCTION_H
