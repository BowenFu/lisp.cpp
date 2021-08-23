#include "evaluator.h"

ExprPtr Lambda::eval(std::shared_ptr<Env> const& env)
{
    CompoundProcedure proc{mBody, mParameters, env};
    return std::shared_ptr<Expr>(new CompoundProcedure(proc));
}

ExprPtr Definition::eval(std::shared_ptr<Env> const& env)
{
    return env->defineVariable(dynamic_cast<Variable*>(mVariable.get())->name(), mValue->eval(env));
}

#if 0
int32_t main()
{
    using Number = Literal<double>;
    auto mul = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        auto result = std::accumulate(args.begin(), args.end(), 1.0, [](auto p, std::shared_ptr<Expr> const& arg)
        {
            auto num = dynamic_cast<Number&>(*arg);
            return p * num.get();
        }
        );
        return std::shared_ptr<Expr>(new Number(result)); 
    };
    auto a = std::shared_ptr<Expr>(new Number(5));
    auto b = std::shared_ptr<Expr>(new Number(0.5));
    Env env{};
    Variable variableA;
    auto defA = Definition(variableA, a);
    defA.eval(env);
    std::cout << variableA.eval(env)->toString() << std::endl;
    auto assignA = Assignment(variableA, b);
    assignA.eval(env);
    std::cout << variableA.eval(env)->toString() << std::endl;
    auto mulOp = ExprPtr{new PrimitiveProcedure{mul}};
    Variable variableMul;
    auto defMul = Definition(variableMul, mulOp);
    defMul.eval(env);
    auto e = dynamic_cast<Procedure&>(*variableMul.eval(env)).apply({a, b});
    std::cout << e->eval(env)->toString() << std::endl;
    return 0;
}
#endif