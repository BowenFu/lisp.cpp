#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"

class Compiler
{
    using SymbolTable = std::map<std::string, std::pair<ExprPtr, size_t>>;
    SymbolTable mSymbolTable{};
    ByteCode mCode{};
    using FuncInfo = std::pair<Instructions, SymbolTable>;
    std::optional<FuncInfo> mFunc{};
    auto& instructions()
    {
        return mFunc ? mFunc.value().first : mCode.instructions;
    }
    // auto& resolveIndex(std::string const& name) const
    // {
    //     return mFunc ? mFuncInstructions.value() : mCode.instructions;
    // }
    // auto& define(std::string const& name)
    // {
    //     return mFunc ? mFuncInstructions.value() : mCode.instructions;
    // }
public:
    Compiler() = default;
    void compile(ExprPtr const& expr);
    ByteCode code() const
    {
        return mCode;
    }
};

#endif // LISP_COMPILER_H