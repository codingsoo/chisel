#ifndef REDUCTION_H
#define REDUCTION_H

#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/Rewrite/Core/Rewriter.h"

using DDElement = llvm::PointerUnion<clang::Decl *, clang::Stmt *>;
using DDElementVector = std::vector<DDElement>;
using DDElementSet = std::set<DDElement>;

class Reduction : public clang::ASTConsumer {
public:
  Reduction() {}
  ~Reduction() {}

protected:
  virtual void Initialize(clang::ASTContext &Ctx);

  void doDeltaDebugging(std::vector<DDElement> &Decls);

  virtual bool test(std::vector<DDElement> &ToBeRemoved) = 0;
  virtual bool callOracle();
  virtual std::vector<DDElementVector>
  refineChunks(std::vector<DDElementVector> &Chunks) = 0;

  void writeToFile(std::string Filename);
  clang::SourceLocation getEndLocationAfter(clang::SourceRange Range,
                                            char Symbol);
  clang::SourceLocation getEndLocationUntil(clang::SourceRange Range,
                                            char Symbol);
  clang::SourceLocation getEndLocationFromBegin(clang::SourceRange Range);
  int getOffsetUntil(const char *Buf, char Symbol);
  std::string getSourceText(clang::SourceRange SR);

  clang::ASTContext *Context;
  clang::Rewriter TheRewriter;

private:
  DDElementSet toSet(DDElementVector &Vec);
  DDElementVector toVector(DDElementSet &Set);
  std::vector<DDElementVector> split(DDElementVector &Vec, int n);
  DDElementSet setDifference(DDElementSet &A, DDElementSet &B);
};

#endif // REDUCTION_H