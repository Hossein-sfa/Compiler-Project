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

    };

    virtual void visit(Loop &Node) override
    {
    };

    virtual void visit(Equation &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
    };

    virtual void visit(Final &Node) override
    {
      
    };

    virtual void visit(Expression &Node) override
    {
    };

    virtual void visit(Define &Node) override
    {
    };

    virtual void visit(Term &Node) override{};

    virtual void visit(Factor &Node) override{};

    virtual void visit(CompOp &Node) override{};
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
