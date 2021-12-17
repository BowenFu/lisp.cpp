#include "lisp/evaluator.h"
#include "lisp/parser.h"

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

ExprPtr Definition::eval(std::shared_ptr<Env> const& env)
{
    return env->defineVariable(mVariableName, mValue->eval(env));
}


ExprPtr MacroProcedure::apply(std::vector<std::shared_ptr<Expr> > const &args)
{
    auto result = CompoundProcedureBase::apply(args);
    return transform(result, [](ExprPtr const& expr)
        {
            if (auto s = dynamic_cast<Symbol const*>(expr.get()))
            {
                return ExprPtr{new RawWord{s->get()}};
            }
            return expr;
        }
    );
}
