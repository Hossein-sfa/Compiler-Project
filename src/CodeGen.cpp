#include "CodeGen.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Define a visitor class for generating LLVM IR from the AST.
namespace
{
  class ToIRVisitor : public ASTVisitor
  {
    Module *M;
    IRBuilder<> Builder;
    Type *VoidTy;
    Type *Int32Ty;
    Type *Int8PtrTy;
    Type *Int8PtrPtrTy;
    Constant *Int32Zero;

    Value *V;
    StringMap<AllocaInst *> nameMap;

  public:
    // Constructor for the visitor class.
    ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
    {
      // Initialize LLVM types and constants.
      VoidTy = Type::getVoidTy(M->getContext());
      Int32Ty = Type::getInt32Ty(M->getContext());
      Int8PtrTy = Type::getInt8PtrTy(M->getContext());
      Int8PtrPtrTy = Int8PtrTy->getPointerTo();
      Int32Zero = ConstantInt::get(Int32Ty, 0, true);
    }

    // Entry point for generating LLVM IR from the AST.
    void run(AST *Tree)
    {
      // Create the main function with the appropriate function type.
      FunctionType *MainFty = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
      Function *MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

      // Create a basic block for the entry point of the main function.
      BasicBlock *BB = BasicBlock::Create(M->getContext(), "entry", MainFn);
      Builder.SetInsertPoint(BB);

      // Visit the root node of the AST to generate IR.
      Tree->accept(*this);

      // Create a return instruction at the end of the main function.
      Builder.CreateRet(Int32Zero);
    }

    // Visit function for the GSM node in the AST.
    virtual void visit(Goal &Node) override
    {
      // Iterate over the children of the GSM node and visit each child.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        (*I)->accept(*this);
      }
    };

    virtual void visit(Condition &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getVars()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getAssignments()->accept(*this);
      Value *Right = V;
    };

    virtual void visit(Loop &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getExprs()->accept(*this);

      // Visit the right-hand side of the binary operation and get its value.
      Node.getConditions()->accept(*this);
    };

    virtual void visit(Assignment &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
    };

    virtual void visit(Final &Node) override
    {
      if (Node.getKind() == Final::Id)
      {
        // If the factor is an identifier, load its value from memory.
        V = Builder.CreateLoad(Int32Ty, nameMap[Node.getVal()]);
      }
      else
      {
        // If the factor is a literal, convert it to an integer and create a constant.
        int intval;
        Node.getVal().getAsInteger(10, intval);
        V = ConstantInt::get(Int32Ty, intval, true);
      }
    };

    virtual void visit(Expression &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case Assignment::Plus:
        V = Builder.CreateNSWAdd(Left, Right);
        break;
      case Assignment::Minus:
        V = Builder.CreateNSWSub(Left, Right);
        break;
      case Assignment::power:
        V = Left;
        int intval;
        Right.getVal().getAsInteger(10, intval);
        for (int i = 0; i < intval - 1; i++)
          V = Builder.CreateNSWMul(V, Left);
        break;
        //     case Assignment:: mod:
        //       int l=Right.getVal().getAsInteger(10);
        //      V = Builder(Left, Right);
        //      break;
      }
    };

    virtual void visit(Define &Node) override
    {
      Value *val = nullptr;

      if (Node.getVars())
      {
        // If there is an expression provided, visit it and get its value.
        Node.getVars()->accept(*this);
        val = V;
      }

      // Iterate over the variables declared in the declaration statement.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        StringRef Var = *I;

        // Create an alloca instruction to allocate memory for the variable.
        nameMap[Var] = Builder.CreateAlloca(Int32Ty);

        // Store the initial value (if any) in the variable's memory location.
        if (val != nullptr)
        {
          Builder.CreateStore(val, nameMap[Var]);
        }
      }
    };

    virtual void visit(Term &Node) override{};

    virtual void visit(Factor &Node) override{};
  };
  }; // namespace

  void CodeGen::compile(AST *Tree)
  {
    // Create an LLVM context and a module.
    LLVMContext Ctx;
    Module *M = new Module("calc.expr", Ctx);

    // Create an instance of the ToIRVisitor and run it on the AST to generate LLVM IR.
    ToIRVisitor ToIR(M);
    ToIR.run(Tree);

    // Print the generated module to the standard output.
    M->print(outs(), nullptr);
  }
