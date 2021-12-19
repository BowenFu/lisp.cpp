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
    // std::cout << "\n## from evaluator.cpp ## : " << result->toString() << std::endl << std::endl;
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

ExprPtr vecToCons(std::vector<ExprPtr> const& vec)
{
    auto result = nil();
    auto vecSize = vec.size();
    auto i = vec.rbegin();
    if (vecSize >= 2)
    {
        auto dot = vec.at(vecSize - 2);
        auto dotPtr = dynamic_cast<RawWord*>(dot.get());
        if (dotPtr != nullptr && dotPtr->toString() == ".")
        {
            ASSERT(vecSize >=3);
            ++i;
            ++i;
            result = vec.back();
        }
    }
    for (;i != vec.rend(); ++i)
    {
        result = ExprPtr{new Cons{*i, result}};
    }
    return result;
}

std::vector<ExprPtr> consToVec(ExprPtr const& expr)
{
    std::vector<ExprPtr> vec;
    auto me = expr;
    while (me != nil())
    {
        auto [car, cdr] = deCons(me);
        vec.push_back(car);
        me = cdr;
    }
    return vec;
}
