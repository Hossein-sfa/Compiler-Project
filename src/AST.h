#ifndef AST_H
#define AST_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

// Forward declarations of classes used in the AST
// class AST;
// class Expr;
// class GSM;
// class Factor;
// class BinaryOp;
// class Assignment;
// class Declaration;

class AST;
class Goal;
class Expr;
class Define;
class Condition;
class CompOp;
class Loop;
class Expression;
class Equation;
class Final;

// ASTVisitor class defines a visitor pattern to traverse the AST
class ASTVisitor
{
public:
  // Virtual visit functions for each AST node type
  virtual void visit(AST &) {}               // Visit the base AST node
  virtual void visit(Expr &) {}              // Visit the expression node
  virtual void visit(Goal &) = 0;             // Visit the group of expressions node       
  virtual void visit(Equation &) = 0;        // Visit the binary operation node
  virtual void visit(Define &) = 0;      // Visit the assignment expression node
  virtual void visit(Final &) = 0;     // Visit the variable declaration node
  virtual void visit(Loop &) = 0;  
  virtual void visit(CompOp &) = 0;
  virtual void visit(Condition &) = 0;
};

// AST class serves as the base class for all AST nodes
class AST
{
public:
  virtual ~AST() {}
  virtual void accept(ASTVisitor &V) = 0;    // Accept a visitor for traversal
};

// Expr class represents an expression in the AST
class Expr : public AST
{
public:
  Expr() {}
};

// GSM class represents a group of expressions in the AST
class Goal : public Expr
{
  using ExprVector = llvm::SmallVector<Expr *>;

private:
  ExprVector exprs;                          // Stores the list of expressions

public:
  Goal(llvm::SmallVector<Expr *> exprs) : exprs(exprs) {}

  llvm::SmallVector<Expr *> getExprs() { return exprs; }

  ExprVector::const_iterator begin() { return exprs.begin(); }

  ExprVector::const_iterator end() { return exprs.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class Final : public Expr
{
  enum ValueKind
  {
    Id,
    Number
  };
  private:
  ValueKind Kind;                            // Stores the kind of factor (identifier or number)
  llvm::StringRef Val;                       // Stores the value of the factor

public:
  Final(ValueKind Kind, llvm::StringRef Val) : Kind(Kind), Val(Val) {}

  ValueKind getKind() { return Kind; }

  llvm::StringRef getVal() { return Val; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// IF ELSE ELIF Compound condition
class Condition : public Expr
{
  using ExprVector = llvm::SmallVector<Expr *>;

private:
  ExprVector exprs;        

public:
  Condition(llvm::SmallVector<Expr *> exprs) : exprs{exprs} {}

  llvm::SmallVector<Expr *> getExprs() { return exprs; }

  ExprVector::const_iterator begin() { return exprs.begin(); }

  ExprVector::const_iterator end() { return exprs.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class Equation : public Expr
{
public:
  enum Operator
  {
    Plus,
    Minus,
    Mul,
    Div,
    power,
    mod,
    minus_equal,
    plus_equal,
    slash_equal,
    mod_equal,
    mul_equal,
    equal,
  };

private:
  Expr *Left;                               // Left-hand side expression
  Expr *Right;                              // Right-hand side expression
  Operator Op;                              // Operator of the binary operation

public:
  Equation(Operator Op, Expr *L, Expr *R) : Op(Op), Left(L), Right(R) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// loopc
class Loop : public Expr
{
  using ExprVector = llvm::SmallVector<Expr *>;

private:
  Expr CompOpCondition;
  ExprVector exprs;

public:
  Loop(llvm::SmallVector<Expr *> exprs, Expr *C) : exprs{exprs}, C(CompOpCondition) {}

  llvm::SmallVector<Expr *> getExprs() { return exprs; }

  ExprVector::const_iterator begin() { return exprs.begin(); }

  ExprVector::const_iterator end() { return exprs.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class Expression : public Expr
{
public:
  enum Operator
  {
    Plus,
    Minus,
  };

private:
  Expr *Left;                               // Left-hand side expression
  Expr *Right;                              // Right-hand side expression
  Operator Op;                              // Operator of the binary operation

public:
  Expression(Operator Op, Expr *L, Expr *R) : Op(Op), Left(L), Right(R) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class CompOp : public Expr
{
public:
  enum Operator
  {
    lte,
    gte,
    is_equal,
    not_equal,
    gt,
    lt,
  };

private:
  Expr *Left;                               // Left-hand side expression
  Expr *Right;                              // Right-hand side expression
  Operator Op;    
  Expr * IF;                          // Operator of the binary operation

public:
  CompOp(Operator Op, Expr *L, Expr *R, Expr *I) : Op(Op), Left(L), Right(R), IF(I) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Expr *getIF() { return IF; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// Declaration class represents a variable declaration with an initializer in the AST
class Define : public Expr
{
  using VarVector = llvm::SmallVector<llvm::StringRef, 8>;
  VarVector vars, exprs;

public:
  Define(llvm::SmallVector<llvm::StringRef, 8> Vars, VarVector ExprVector) : Vars(Vars), exprs(exprs) {}

  llvm::SmallVector<llvm::StringRef, 8> getVars() { return vars; }

  llvm::SmallVector<llvm::StringRef, 8> getExprs() { return exprs; }

  VarVector::const_iterator begin() { return vars.begin(); }

  VarVector::const_iterator end() { return vars.end(); }

  VarVector::const_iterator final_begin() { return exprs.begin(); }
  
  VarVector::const_iterator final_end() { return exprs.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

#endif
