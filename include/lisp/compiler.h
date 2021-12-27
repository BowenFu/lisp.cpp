#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"

class Compiler
{
    Env mEnv{};
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