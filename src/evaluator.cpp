#include "lisp/evaluator.h"

ExprPtr true_()
{
    static ExprPtr t{new Bool{true}};
    return t;
}

ExprPtr false_()
{
    static ExprPtr f{new Bool{false}};
    return f;
}

ExprPtr nil()
{
    static ExprPtr n{new Nil{}};
    return n;
}

ExprPtr Lambda::eval(std::shared_ptr<Env> const& env)
{
    CompoundProcedure proc{mBody, mParameters, mVariadic, env};
    return std::shared_ptr<Expr>(new CompoundProcedure(proc));
}

ExprPtr Definition::eval(std::shared_ptr<Env> const& env)
{
    return env->defineVariable(mVariableName, mValue->eval(env));
}
