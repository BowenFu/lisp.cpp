#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"

class SymbolTable
{
public:
    std::map<std::string, std::pair<ExprPtr, size_t>> symbolTable{};
    std::shared_ptr<SymbolTable> outer{};
};

class Compiler
{
    std::shared_ptr<SymbolTable> mSymbolTable = std::make_shared<SymbolTable>();
    ByteCode mCode{};
public:
    Compiler() = default;
    void compile(ExprPtr const& expr);
    ByteCode code() const
    {
        return mCode;
    }
};

#endif // LISP_COMPILER_H