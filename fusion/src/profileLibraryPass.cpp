#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

int main(int argc, char *argv[]){
  LLVMContext Context;
  Module *M = new Module("profiling Function", Context);
  SmallVector<Type*,1> mainInputs;
  
  Function *f;
  FunctionType *fTy;
  BasicBlock *BB;
  Value *ret;
  vector<Function*> funcs;

  IRBuilder<> Builder(Context);
  // Unary Integer
  // Binary Integer
  SmallVector<Type*,30> vInputs;
  SmallVector<Type*,30> vOutputs;
  SmallVector<Type*,2> int32Input2;
  SmallVector<Type*,2> float32Input2;
  int32Input2.push_back(Type::getInt32Ty(Context));
  FunctionType *int3264Ty = FunctionType::get(Type::getInt64Ty(Context),int32Input2,false);
  FunctionType *inttoptrTy = FunctionType::get(Type::getInt32PtrTy(Context),int32Input2,false);
  FunctionType *inttofpTy = FunctionType::get(Type::getFloatTy(Context),int32Input2,false);
  int32Input2.push_back(Type::getInt32Ty(Context));
  FunctionType *int32fTy = FunctionType::get(Type::getInt32Ty(Context),int32Input2,false);
  FunctionType *int321fTy = FunctionType::get(Type::getInt1Ty(Context),int32Input2,false);
  SmallVector<Type*,3> int1Input132Input2;
  int1Input132Input2.push_back(Type::getInt1Ty(Context));
  int1Input132Input2.push_back(Type::getInt32Ty(Context));
  int1Input132Input2.push_back(Type::getInt32Ty(Context));
  FunctionType *int132fTy = FunctionType::get(Type::getInt32Ty(Context),int1Input132Input2,false);
  float32Input2.push_back(Type::getFloatTy(Context));
  FunctionType *float132fTy = FunctionType::get(Type::getFloatTy(Context),float32Input2,false);
  FunctionType *fptouiTy = FunctionType::get(Type::getInt32Ty(Context),float32Input2,false);
  FunctionType *fpextTy = FunctionType::get(Type::getDoubleTy(Context),float32Input2,false);
  float32Input2.push_back(Type::getFloatTy(Context));
  FunctionType *float32fTy = FunctionType::get(Type::getFloatTy(Context),float32Input2,false);
  SmallVector<Type*,3> float1Input132Input2;
  float1Input132Input2.push_back(Type::getFloatTy(Context));
  float1Input132Input2.push_back(Type::getFloatTy(Context));
  FunctionType *float321fTy = FunctionType::get(Type::getInt1Ty(Context),float1Input132Input2,false);

  SmallVector<Type*,1> ptrTy;
  ptrTy.push_back(Type::getInt32PtrTy(Context));
  FunctionType *ptrintTy = FunctionType::get(Type::getInt32Ty(Context),ptrTy,false);
  
  SmallVector<Type*,2> elemTy;
  //elemTy.push_back(VectorType::get(Type::getInt32Ty(Context),4,false)->getPointerTo());
  elemTy.push_back(Type::getInt32PtrTy(Context));
  elemTy.push_back(Type::getInt32Ty(Context));
  FunctionType *getelemTy = FunctionType::get(Type::getInt32PtrTy(Context),elemTy,false);
  
  SmallVector<Type*,1> input64;
  input64.push_back(Type::getInt64Ty(Context));
  FunctionType *truncTy = FunctionType::get(Type::getInt32Ty(Context),input64,false);

  SmallVector<Type*,1> doubleInput;
  doubleInput.push_back(Type::getDoubleTy(Context));
  FunctionType *fptruncTy = FunctionType::get(Type::getFloatTy(Context),doubleInput,false);
  
  // Add
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_addi",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateAdd(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Sub
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_subi",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSub(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Mul
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_muli",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateMul(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // UDiv
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_udiv",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateUDiv(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // SDiv
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_sdiv",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSDiv(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // URem
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_urem",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateURem(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // SRem
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_srem",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSRem(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Shl
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_shl",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateShl(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // LShr
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_lshr",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateLShr(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  //Ashr
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_ashr",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateAShr(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // And
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_and",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateAnd(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Or
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_or",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateOr(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Xor
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int32fTy,Function::ExternalLinkage,"profile_xor",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateXor(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Select
  vInputs.push_back(Type::getInt1Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(int132fTy,Function::ExternalLinkage,"profile_select",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSelect(f->getArg(0),f->getArg(1),f->getArg(2));
  Builder.CreateRet(ret);
  // Icmp EQ
  vInputs.push_back(Type::getInt32Ty(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt1Ty(Context));
  f = Function::Create(int321fTy,Function::ExternalLinkage,"profile_icmp",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateICmpEQ(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // FCmp EQ
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getInt1Ty(Context));
  f = Function::Create(float321fTy,Function::ExternalLinkage,"profile_fcmp",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFCmpUEQ(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Zext
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt64Ty(Context));
  f = Function::Create(int3264Ty,Function::ExternalLinkage,"profile_zext",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateZExt(f->getArg(0),Type::getInt64Ty(Context));
  Builder.CreateRet(ret);
  // Sext
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt64Ty(Context));
  f = Function::Create(int3264Ty,Function::ExternalLinkage,"profile_sext",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSExt(f->getArg(0),Type::getInt64Ty(Context));
  Builder.CreateRet(ret);
  // FPtoUI
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(fptouiTy,Function::ExternalLinkage,"profile_fptoui",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFPToUI(f->getArg(0),Type::getInt32Ty(Context));
  Builder.CreateRet(ret);
  // FPToSI
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(fptouiTy,Function::ExternalLinkage,"profile_fptosi",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFPToSI(f->getArg(0),Type::getInt32Ty(Context));
  Builder.CreateRet(ret);
  // FPExt
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getDoubleTy(Context));
  f = Function::Create(fpextTy,Function::ExternalLinkage,"profile_fpext",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFPExt(f->getArg(0),Type::getDoubleTy(Context));
  Builder.CreateRet(ret);
  // PtrToInt
  vInputs.push_back(Type::getInt32PtrTy(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(ptrintTy,Function::ExternalLinkage,"profile_ptrtoint",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreatePtrToInt(f->getArg(0),Type::getInt32Ty(Context));
  Builder.CreateRet(ret);
  // IntToPtr
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32PtrTy(Context));
  f = Function::Create(inttoptrTy,Function::ExternalLinkage,"profile_inttoptr",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateIntToPtr(f->getArg(0),Type::getInt32Ty(Context));
  Builder.CreateRet(ret);
  // SIToFP
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(inttofpTy,Function::ExternalLinkage,"profile_sitofp",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateSIToFP(f->getArg(0),Type::getFloatTy(Context));
  Builder.CreateRet(ret);
  // UIToFP
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(inttofpTy,Function::ExternalLinkage,"profile_uitofp",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateUIToFP(f->getArg(0),Type::getFloatTy(Context));
  Builder.CreateRet(ret);
  // Trunc
  vInputs.push_back(Type::getInt64Ty(Context));
  vOutputs.push_back(Type::getInt32Ty(Context));
  f = Function::Create(truncTy,Function::ExternalLinkage,"profile_trunc",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateTrunc(f->getArg(0),Type::getInt32Ty(Context));
  Builder.CreateRet(ret);
  // FPTrunc
  vInputs.push_back(Type::getDoubleTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(fptruncTy,Function::ExternalLinkage,"profile_fptrunc",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFPTrunc(f->getArg(0),Type::getFloatTy(Context));
  Builder.CreateRet(ret);
  // BitCast
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(inttofpTy,Function::ExternalLinkage,"profile_bitcast",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateBitCast(f->getArg(0),Type::getFloatTy(Context));
  Builder.CreateRet(ret);
  // Load
  // Store
  // GetElementPtr
  vInputs.push_back(Type::getInt32PtrTy(Context));
  vInputs.push_back(Type::getInt32Ty(Context));
  vOutputs.push_back(Type::getInt32PtrTy(Context));
  vector<Value*> index_vector;
  f = Function::Create(getelemTy,Function::ExternalLinkage,"profile_getelementptr",M);
  index_vector.push_back(f->getArg(1));
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateGEP(f->getArg(0),index_vector);
  Builder.CreateRet(ret);
  // FNeg
   /*
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float132fTy,Function::ExternalLinkage,"profile_fneg",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFNeg(f->getArg(0));
  Builder.CreateRet(ret);*/
  // Fadd
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float32fTy,Function::ExternalLinkage,"profile_fadd",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFAdd(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // Fsub
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float32fTy,Function::ExternalLinkage,"profile_fsub",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFSub(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // FMul
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float32fTy,Function::ExternalLinkage,"profile_fmul",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFMul(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // FDiv
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float32fTy,Function::ExternalLinkage,"profile_fdiv",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFDiv(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // FRem
  vInputs.push_back(Type::getFloatTy(Context));
  vInputs.push_back(Type::getFloatTy(Context));
  vOutputs.push_back(Type::getFloatTy(Context));
  f = Function::Create(float32fTy,Function::ExternalLinkage,"profile_frem",M);
  f->addFnAttr(Attribute::NoInline);
  funcs.push_back(f);
  BB = BasicBlock::Create(Context,"entry",f);
  Builder.SetInsertPoint(BB);
  ret = Builder.CreateFRem(f->getArg(0),f->getArg(1));
  Builder.CreateRet(ret);
  // ShuffleVector
  // ExtractElement
  // InsertElement
  // InsertValue

  // Other Ops Integer
  //
  //
  StructType *outputStruct = StructType::create(Context, vOutputs);
  //Value *RetVal = M->getOrInsertGlobal("output",outputStruct);
  FunctionType *fmainTy = FunctionType::get(outputStruct,vInputs,false);
  Function *fmain = Function::Create(fmainTy,Function::ExternalLinkage,"top",M);
  BasicBlock *mainBB = BasicBlock::Create(Context,"entry",fmain);
  Builder.SetInsertPoint(mainBB);
  Value *RetVal = Builder.CreateAlloca(outputStruct);

  int input_index = 0;
  for(int i = 0; i <  funcs.size(); ++i){
    SmallVector<Value*,2> inputs;
    for(int j=0; j < funcs[i]->arg_size(); ++j){
      inputs.push_back(fmain->getArg(input_index++));
    }
    Value *Out = Builder.CreateCall(funcs[i],inputs);
    Value *GEP = Builder.CreateStructGEP(RetVal,i);
    Builder.CreateStore(Out,GEP,true);

  }
  
  Builder.CreateRet(RetVal);
  WriteBitcodeToFile(*M,outs());


  return 0;
}
