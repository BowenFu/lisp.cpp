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

ExprPtr Lambda::eval(std::shared_ptr<Env> const& env)
{
    CompoundProcedure proc{mBody, mParameters, env};
    return std::shared_ptr<Expr>(new CompoundProcedure(proc));
}

ExprPtr Definition::eval(std::shared_ptr<Env> const& env)
{
    return env->defineVariable(dynamic_cast<Variable*>(mVariable.get())->name(), mValue->eval(env));
}
