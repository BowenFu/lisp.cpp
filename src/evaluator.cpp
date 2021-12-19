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

auto expandMacros(ExprPtr const& expr, std::shared_ptr<Env> const& env) -> ExprPtr
{
    if (auto macroDefinition = parseMacroDefinition(expr))
    {
        macroDefinition->eval(env);
        return nil();
    }
    if (auto e = tryMacroCall(expr, env))
    {
        if (e != expr)
        {
            return e;
        }
    }
    if (auto c = dynamic_cast<Cons const*>(expr.get()))
    {
        auto carResult = expandMacros(c->car(), env);
        auto cdrResult = expandMacros(c->cdr(), env);
        if (carResult == c->car() && cdrResult == c->cdr())
        {
            return expr;
        }
        return ExprPtr{new Cons{carResult, cdrResult}};
    }
    return expr;
}
