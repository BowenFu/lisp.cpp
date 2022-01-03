#include "lisp/evaluator.h"
#include <iostream>
#include <cmath>
#include <numeric>

auto consOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    return ExprPtr{new Cons{args.at(0), args.at(1)}}; 
};

auto printOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    for (auto const& e: args)
    {
        std::cout << e->toString();
    }
    std::cout << std::endl;
    return null(); 
};

auto errorOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    std::stringstream o;
    for (auto const& e: args)
    {
        o << e->toString() << " ";
    }
    o << std::endl;
    throw std::runtime_error{o.str()};
    return null(); 
};

auto consPred = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    if (dynamic_cast<Cons const*>(args.front().get()))
    {
        return true_();
    }
    return false_();
};

auto carOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons*>(args.at(0).get());
    ASSERT(cons_);
    return cons_->car(); 
};

auto cdrOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto cons_ = dynamic_cast<Cons&>(*args.at(0));
    return cons_.cdr(); 
};

auto isNullOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 1);
    auto result = (args.at(0) == null());
    return result ? true_() : false_(); 
};

auto modOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    auto lhsNum = dynamic_cast<Number*>(args.at(0).get());
    ASSERT(lhsNum);
    auto lhsD = lhsNum->get();
    auto rhsNum = dynamic_cast<Number*>(args.at(1).get());
    ASSERT(rhsNum);
    auto rhsD = rhsNum->get();
    ASSERT(std::trunc(lhsD) == lhsD);
    ASSERT(std::trunc(rhsD) == rhsD);
    double result = static_cast<int32_t>(lhsD) % static_cast<int32_t>(rhsD);
    return ExprPtr{new Number{result}}; 
};

auto isEqOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    return (args.at(0) == args.at(1) || (args.at(0)->equalTo(args.at(1)))) ? true_() : false_(); 
};

template <typename Func>
constexpr auto numBinOp(Func func)
{
    return [func](std::vector<std::shared_ptr<Expr> > const &args)
    {
        ASSERT(args.size() == 2);
        auto lhs = args.at(0);
        auto rhs = args.at(1);
        auto lhsN = dynamic_cast<Number *>(lhs.get());
        auto rhsN = dynamic_cast<Number *>(rhs.get());
        ASSERT(lhsN);
        ASSERT(rhsN);
        return func(lhsN->get(), rhsN->get()) ? true_() : false_();
    };
}

constexpr auto lessOp = numBinOp(std::less<>());

auto mulOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    auto result = std::accumulate(args.begin(), args.end(), 1.0, [](auto p, std::shared_ptr<Expr> const& arg)
    {
        auto num = dynamic_cast<Number&>(*arg);
        return p * num.get();
    }
    );
    return std::shared_ptr<Expr>(new Number(result)); 
};

auto addOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    auto result = std::accumulate(args.begin(), args.end(), 0.0, [](auto p, std::shared_ptr<Expr> const& arg)
    {
        auto num = dynamic_cast<Number&>(*arg);
        return p + num.get();
    }
    );
    return std::shared_ptr<Expr>(new Number(result)); 
};

auto divOp = [](std::vector<std::shared_ptr<Expr>> const& args)
{
    ASSERT(args.size() == 2);
    auto num1 = dynamic_cast<Number&>(*args.at(0));
    auto num2 = dynamic_cast<Number&>(*args.at(1));
    return std::shared_ptr<Expr>(new Number(num1.get() / num2.get())); 
};

std::shared_ptr<Env> setUpEnvironment()
{
    auto emptyEnv = std::make_shared<Env>();
    auto primitiveProcedureNames = std::vector<std::string>{};
    auto primitiveProcedureObjects = std::vector<ExprPtr>{};
    auto initialEnv = emptyEnv->extend(Params{std::make_pair(primitiveProcedureNames, false)}, primitiveProcedureObjects);

    initialEnv->defineVariable("cons", ExprPtr{new PrimitiveProcedure{consOp}});
    initialEnv->defineVariable("cons?", ExprPtr{new PrimitiveProcedure{consPred}});
    initialEnv->defineVariable("print", ExprPtr{new PrimitiveProcedure{printOp}});
    initialEnv->defineVariable("error", ExprPtr{new PrimitiveProcedure{errorOp}});
    initialEnv->defineVariable("car", ExprPtr{new PrimitiveProcedure{carOp}});
    initialEnv->defineVariable("cdr", ExprPtr{new PrimitiveProcedure{cdrOp}});
    initialEnv->defineVariable("null?", ExprPtr{new PrimitiveProcedure{isNullOp}});
    initialEnv->defineVariable("eq?", ExprPtr{new PrimitiveProcedure{isEqOp}});
    initialEnv->defineVariable("%", ExprPtr{new PrimitiveProcedure{modOp}});
    initialEnv->defineVariable("=", ExprPtr{new PrimitiveProcedure{isEqOp}});
    initialEnv->defineVariable("<", ExprPtr{new PrimitiveProcedure{lessOp}});
    initialEnv->defineVariable("+", ExprPtr{new PrimitiveProcedure{addOp}});
    initialEnv->defineVariable("*", ExprPtr{new PrimitiveProcedure{mulOp}});
    initialEnv->defineVariable("/", ExprPtr{new PrimitiveProcedure{divOp}});

    initialEnv->defineVariable("null", null());
    return initialEnv;
}
