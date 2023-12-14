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
    Function *MainFn;
    FunctionType *MainFty;

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
      MainFty = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
      MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

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

    virtual void visit(Assignment &Node) override
    {
      // Visit the right-hand side of the assignment and get its value.
      Node.getRight()->accept(*this);
      Value *val = V;

      // Get the name of the variable being assigned.
      auto varName = Node.getLeft()->getVal();

      // Create a store instruction to assign the value to the variable.
      Builder.CreateStore(val, nameMap[varName]);

      // Create a function type for the "gsm_write" function.
      FunctionType *CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);

      // Create a function Define for the "gsm_write" function.
      Function *CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "gsm_write", M);

      // Create a call instruction to invoke the "gsm_write" function with the value.
      CallInst *Call = Builder.CreateCall(CalcWriteFnTy, CalcWriteFn, {val});
    };

    virtual void visit(Factor &Node) override
    {
      if (Node.getKind() == Factor::Id)
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

    virtual void visit(BinaryOp &Node) override
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
      case BinaryOp::Plus:
        V = Builder.CreateNSWAdd(Left, Right);
        break;
      case BinaryOp::Minus:
        V = Builder.CreateNSWSub(Left, Right);
        break;
      case BinaryOp::Mul:
        V = Builder.CreateNSWMul(Left, Right);
        break;
      case BinaryOp::Div:
        V = Builder.CreateSDiv(Left, Right);
        break;
      case BinaryOp::power:
      {
        V = Left;
        Factor *f = (Factor *)Right;
        int temp;
        f->getVal().getAsInteger(10, temp);
        // llvm::errs() << temp
        //      << " :: " << f->getVal();
        if (f && f->getKind() == Factor::ValueKind::Number){
          int right_value_as_int;
          f->getVal().getAsInteger(10, right_value_as_int);
          if(right_value_as_int == 0)
            V = ConstantInt::get(Int32Ty, 1, true);
          else{
            for(int i = 1;i < right_value_as_int;i++){
              V = Builder.CreateNSWMul(V, Left);
            }
          }
        }
        break;
      }
      case BinaryOp::mod:
        V = Builder.CreateSRem(Left, Right);
        break;
      case BinaryOp::OR:
        V = Builder.CreateOr(Left, Right);
        break;
      case BinaryOp::AND:
        V = Builder.CreateAnd(Left, Right);
        break;
      case BinaryOp::is_equal:
        V = Builder.CreateICmpEQ(Left, Right);
        break;
      case BinaryOp::not_equal:
        V = Builder.CreateICmpNE(Left, Right);
        break;
      case BinaryOp::lte:
        V = Builder.CreateICmpSGE(Left, Right);
        break;
      case BinaryOp::gte:
        V = Builder.CreateICmpSLE(Left, Right);
        break;
      case BinaryOp::lt:
        V = Builder.CreateICmpSLT(Left, Right);
        break;
      case BinaryOp::gt:
        V = Builder.CreateICmpSGT(Left, Right);
        break;
      }
    };

    virtual void visit(Define &Node) override
    {

      // Our code
      bool hasValue = false;
      auto e_I = Node.begin_values(), e_E = Node.end_values();
      // Iterate over the variables declared in the Define statement.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I, ++e_I)
      {
        StringRef Var = *I;
        Value *val = nullptr;
        // Create an alloca instruction to allocate memory for the variable.
        nameMap[Var] = Builder.CreateAlloca(Int32Ty);
        
        if (e_I != e_E) // star or not ? 
        {
          (* e_I)->accept(*this);

          val = V;

          if (val != nullptr)
          {
            Builder.CreateStore(val, nameMap[Var]);
          }
        }
        else if(e_I == e_E || hasValue)
        {
          hasValue = true;
          val = ConstantInt::get(Int32Ty, 0, true);
          if (val != nullptr)
          {
            Builder.CreateStore(val, nameMap[Var]);
          }
        }
      }
    };

    virtual void visit(IF &Node) override
    {
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        if (*I)
        {
          Assignment *casted_I = (Assignment *)I;
          casted_I->accept(*this);
        }
      }
    };

    virtual void visit(Loop &Node) override
    {
      llvm::BasicBlock* WhileCondBB = llvm::BasicBlock::Create(M->getContext(), "loopc.cond", MainFn);
      llvm::BasicBlock* WhileBodyBB = llvm::BasicBlock::Create(M->getContext(), "loopc.body", MainFn);
      llvm::BasicBlock* AfterWhileBB = llvm::BasicBlock::Create(M->getContext(), "after.loopc", MainFn);

      Builder.CreateBr(WhileCondBB);
      Builder.SetInsertPoint(WhileCondBB);
      Node.getExpr()->accept(*this);
      Value* val=V;
      Builder.CreateCondBr(val, WhileBodyBB, AfterWhileBB);
      Builder.SetInsertPoint(WhileBodyBB);
      IF *F = Node.getIF();
      
      for (auto I = F->begin(), E = F->end(); I != E; ++I){
        (*I)->accept(*this);
      }

      Builder.CreateBr(WhileCondBB);
      Builder.SetInsertPoint(AfterWhileBB);
    

    };

    virtual void visit(Condition &Node) override
    {

      // Our code
      Value *val = nullptr;
      int count_exprs = 0;
      int count_Assignments = 0;
      bool hasIf = true;
      bool endOfCondition = false;
      llvm::BasicBlock* ifcondBB;
      llvm::BasicBlock* ifBodyBB;
      for (auto I = Node.exprs_begin(), E = Node.exprs_end(); I != E; ++I)
      {
        count_exprs++;
      }
      for (auto I = Node.Assignments_begin(), E = Node.Assignments_end(); I != E; ++I)
      {
        count_Assignments++;
      }
      BE* Assignments_I = *(Node.getAllAssignments().begin()), *Assignments_E = *(Node.getAllAssignments().end());
      for (auto I = Node.exprs_begin(), E = Node.exprs_begin(); I != E; ++I, ++Assignments_I)
      {
    
        if (hasIf)
        {
          ifcondBB = llvm::BasicBlock::Create(M -> getContext(), "if.condition", MainFn);
          
          (*I)->accept(*this);
          val = V;
          
          ifBodyBB = llvm::BasicBlock::Create(M -> getContext(), "if.Body", MainFn);
          hasIf = false;

          for (auto F = Assignments_I->begin(), G = Assignments_I->end(); G != F; ++F){
            (*F)->accept(*this);
          }

        }
      }
      // else
      if (count_Assignments + 1 == count_exprs)
      {
        auto Assignmentse = Node.Assignments_end();
        (*Assignmentse)->accept(*this);
      }
      // if(endOfCondition)
    };
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