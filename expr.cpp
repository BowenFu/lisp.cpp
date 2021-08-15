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
    std::shared_ptr<Env> mEnclosingEnvironment;
public:
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
            env = env->mEnclosingEnvironment.get();
        }
        assert(false);
    }
    ExprPtr setVariableValue(Variable const* variable, ExprPtr value);
    ExprPtr defineVariable(Variable const* variable, ExprPtr value)
    {
        auto iter = mFrame.find(variable);
        assert(iter == mFrame.end());
        mFrame.insert({variable, value});
        return value;
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
    std::shared_ptr<Variable> mVariable;
    std::shared_ptr<Expr> mValue;
public:
    ExprPtr eval(Env& env) override
    {
        return env.setVariableValue(mVariable.get(), mValue->eval(env));
    }
    std::string toString() const override;
};

ExprPtr Env::setVariableValue(Variable const* variable, ExprPtr value)
{
    return value;
}

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
    std::shared_ptr<Expr> mPredicate;
    std::shared_ptr<Expr> mConsequent;
    std::shared_ptr<Expr> mAlternative;
public:
    ExprPtr eval(Env& env) override
    {
        return isTrue(mPredicate->eval(env)) ? mConsequent->eval(env) : mAlternative->eval(env);
    }
    std::string toString() const override;
};

class Lambda final : public Expr
{
    std::shared_ptr<Expr> mParameters;
    std::shared_ptr<Expr> mBody;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override;
};

class Sequence final : public Expr
{
    std::shared_ptr<Expr> mActions;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override;
};

class Cond final : public Expr
{
    std::shared_ptr<Expr> mClauses;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override;
};

class Application final : public Expr
{
    std::shared_ptr<Expr> mOperator;
    std::shared_ptr<Expr> mOperands;
public:
    ExprPtr eval(Env& env) override;
    std::string toString() const override;
};

class Procedure
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
    std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) override
    {
        return mImplementation(args);
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
    Variable variable;
    Env env{};
    auto def = Definition(variable, a);
    def.eval(env);
    std::cout << variable.eval(env)->toString() << std::endl;
    auto mulOp = PrimitiveProcedure{mul};
    auto e = mulOp.apply({a, b});
    std::cout << e->eval(env)->toString() << std::endl;
    return 0;
}