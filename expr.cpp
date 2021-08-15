#include <cstdlib>
#include <memory>
#include <sstream>
#include <variant>
#include <iostream>
#include <numeric>
#include <vector>
#include <map>

class Expr;
using ExprPtr = std::shared_ptr<Expr>;

class Variable;

class Env
{
    std::map<Variable const*, ExprPtr> mFrame;
    Env* mEnclosingEnvironment;
public:
    Env() = default;
    Env(std::map<Variable const*, ExprPtr> frame, Env* enclosingEnvironment)
    : mFrame{frame}
    , mEnclosingEnvironment{enclosingEnvironment}
    {}
    ExprPtr lookupVariableValue(Variable const* variable)
    {
        Env* env = this;
        while (env)
        {
            auto iter = env->mFrame.find(variable);
            if (iter != env->mFrame.end())
            {
                return iter->second;
            }
            env = env->mEnclosingEnvironment;
        }
        assert(false);
    }
    ExprPtr setVariableValue(Variable const* variable, ExprPtr value)
    {
        auto iter = mFrame.find(variable);
        assert(iter != mFrame.end());
        iter->second = value;
        return value;
    }
    ExprPtr defineVariable(Variable const* variable, ExprPtr value)
    {
        auto iter = mFrame.find(variable);
        assert(iter == mFrame.end());
        mFrame.insert({variable, value});
        return value;
    }
    std::shared_ptr<Env> extend(std::vector<Variable const*> const& parameters, std::vector<ExprPtr> const& arguments)
    {
        assert(parameters.size() == arguments.size());
        std::map<Variable const*, ExprPtr> frame;
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            frame.insert({parameters.at(i), arguments.at(i)});
        }
        
        return std::make_shared<Env>(frame, this);
    }
};


class Expr
{
public:
    virtual ExprPtr eval(Env& env) = 0;
    virtual std::string toString() const = 0;
    virtual ~Expr() = default;
};

template <typename Value>
class Literal : public Expr
{
    Value mValue;
public:
    Literal(Value value)
    : mValue{value}
    {}
    ExprPtr eval(Env& env) override
    {
        return ExprPtr{new Literal(mValue)};
    }
    std::string toString() const override
    {
        std::ostringstream o;
        o << mValue;
        return o.str();
    }
    Value get() const
    {
        return mValue;
    }
};

class Variable final : public Expr
{
public:
    ExprPtr eval(Env& env) override
    {
        return env.lookupVariableValue(this);
    }
    std::string toString() const override
    {
        return "variable";
    }
};

class Quoted final : public Expr
{
    ExprPtr mContent;
public:
    ExprPtr eval(Env& env) override
    {
        return mContent;
    }
    std::string toString() const override;
};

class Assignment final : public Expr
{
    Variable const& mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Assignment(Variable const& var, std::shared_ptr<Expr> value)
    : mVariable{var}
    , mValue{value}
    {
    }
    ExprPtr eval(Env& env) override
    {
        return env.setVariableValue(&mVariable, mValue->eval(env));
    }
    std::string toString() const override
    {
        return "Assignment";
    }
};

class Definition final : public Expr
{
    Variable const& mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Definition(Variable const& var, std::shared_ptr<Expr> value)
    : mVariable{var}
    , mValue{value}
    {
    }
    ExprPtr eval(Env& env) override
    {
        return env.defineVariable(&mVariable, mValue->eval(env));
    }
    std::string toString() const override
    {
        return "Definition";
    }
};

bool isTrue(ExprPtr expr)
{
    return true;
}

class If final : public Expr
{
    ExprPtr mPredicate;
    ExprPtr mConsequent;
    ExprPtr mAlternative;
public:
    ExprPtr eval(Env& env) override
    {
        return isTrue(mPredicate->eval(env)) ? mConsequent->eval(env) : mAlternative->eval(env);
    }
    std::string toString() const override;
};

class Sequence final : public Expr
{
    std::vector<ExprPtr> mActions;
public:
    ExprPtr eval(Env& env) override
    {
        assert(mActions.size() > 1);
        for (size_t i = 0; i < mActions.size() - 1; i++)
        {
            mActions.at(i)->eval(env);
        }
        return mActions.back()->eval(env);
    }
    std::string toString() const override;
};

class Lambda final : public Expr
{
    std::vector<Variable const*> mParameters;
    std::shared_ptr<Sequence> mBody;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override
    {
        return "Lambda";
    }
};

class Cond final : public Expr
{
    std::shared_ptr<Expr> mClauses;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override;
};

std::vector<ExprPtr> listOfValues(std::vector<ExprPtr> const& exprs, Env& env)
{
    std::vector<ExprPtr> values;
    std::transform(exprs.begin(), exprs.end(), std::back_insert_iterator(values), [&env](ExprPtr const& e)
    {
        return e->eval(env);
    }
    );
    return values;
}

class Procedure : public Expr
{
public:
    virtual std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) = 0;
};

class PrimitiveProcedure : public Procedure
{
    std::function<std::shared_ptr<Expr>(std::vector<std::shared_ptr<Expr>> const& args)> mImplementation;
public:
    template <typename Func>
    PrimitiveProcedure(Func func)
    : mImplementation{func}
    {}
    ExprPtr eval(Env& env) override
    {
        return ExprPtr{new PrimitiveProcedure{mImplementation}};
    }
    std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) override
    {
        return mImplementation(args);
    }
    std::string toString() const override
    {
        return "PrimitiveProcedure";
    }
};

class CompoundProcedure : public Procedure
{
    std::shared_ptr<Sequence> mBody;
    std::vector<Variable const*> mParameters;
    std::shared_ptr<Env> mEnvironment;
public:
    CompoundProcedure(std::shared_ptr<Sequence> body, std::vector<Variable const*> parameters, std::shared_ptr<Env> environment)
    : mBody{body}
    , mParameters{parameters}
    , mEnvironment{environment}
    {}
    ExprPtr eval(Env& env) override
    {
        return ExprPtr{new CompoundProcedure{mBody, mParameters, mEnvironment}};
    }
    std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) override
    {
        return mBody->eval(*mEnvironment->extend(mParameters, args));
    }
    std::string toString() const override
    {
        return "CompoundProcedure";
    }
};

ExprPtr Lambda::eval(Env& env)
{
    CompoundProcedure proc{mBody, mParameters, std::make_shared<Env>(env)};
    return std::shared_ptr<Expr>(new CompoundProcedure(proc));
}

class Application final : public Expr
{
    std::shared_ptr<Expr> mOperator;
    std::vector<ExprPtr> mOperands;
public:
    ExprPtr eval(Env& env) override
    {
        auto op = mOperator->eval(env);
        auto args = listOfValues(mOperands, env);
        return dynamic_cast<Procedure&>(*op).apply(args);
    }
    std::string toString() const override
    {
        return "Application";
    }
};

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