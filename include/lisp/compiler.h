#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"

class Compiler
{
    std::map<std::string, std::pair<ExprPtr, size_t>> mSymbolTable;
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