#include <cstdlib>
#include <memory>
#include <sstream>
#include <variant>
#include <iostream>
#include <numeric>
#include <vector>

class Expr;
class Variable;

class Env
{
public:
    Expr& lookupVariableValue(Variable& variable);
    Expr& setVariableValue(Variable& variable, Expr& value);
    Expr& defineVariable(Variable& variable, Expr& value);
};


class Expr
{
public:
    virtual Expr& eval(Env& env) = 0;
    virtual std::string toString() const = 0;
    virtual ~Expr() = default;
};

class Literal : public Expr
{
public:
    Expr& eval(Env& env) override
    {
        return *this;
    }
};

class Number : public Literal
{
public:
    std::variant<int32_t, double> mNumber;
    template <typename N>
    Number(N n)
    : mNumber(n)
    {}
    std::string toString() const override
    {
        std::ostringstream o;
        switch (mNumber.index())
        {
        case 0:
            o << std::get<0>(mNumber);
            break;
        
        case 1:
            o << std::get<1>(mNumber);
            break;
        }
        return o.str();
    }
};

class Variable final : public Expr
{
public:
    Expr& eval(Env& env) override
    {
        return env.lookupVariableValue(*this);
    }
    std::string toString() const override;
};

Expr& Env::lookupVariableValue(Variable& variable)
{
    return variable;
}

class Quoted final : public Expr
{
    std::unique_ptr<Expr> mContent;
public:
    Expr& eval(Env& env) override
    {
        return *mContent;
    }
    std::string toString() const override;
};

class Assignment final : public Expr
{
    std::shared_ptr<Variable> mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Expr& eval(Env& env) override
    {
        return env.setVariableValue(*mVariable, mValue->eval(env));
    }
    std::string toString() const override;
};

Expr& Env::setVariableValue(Variable& variable, Expr& value)
{
    return value;
}

class Definition final : public Expr
{
    std::shared_ptr<Variable> mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Expr& eval(Env& env) override
    {
        return env.defineVariable(*mVariable, mValue->eval(env));
    }
    std::string toString() const override;
};

Expr& Env::defineVariable(Variable& variable, Expr& value)
{
    return value;
}

bool isTrue(Expr& expr)
{
    return true;
}

class If final : public Expr
{
    std::shared_ptr<Expr> mPredicate;
    std::shared_ptr<Expr> mConsequent;
    std::shared_ptr<Expr> mAlternative;
public:
    Expr& eval(Env& env) override
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
    Expr& eval(Env& env) override;
    std::string toString() const override;
};

class Sequence final : public Expr
{
    std::shared_ptr<Expr> mActions;
public:
    Expr& eval(Env& env) override;
    std::string toString() const override;
};

class Cond final : public Expr
{
    std::shared_ptr<Expr> mClauses;
public:
    Expr& eval(Env& env) override;
    std::string toString() const override;
};

class Application final : public Expr
{
    std::shared_ptr<Expr> mOperator;
    std::shared_ptr<Expr> mOperands;
public:
    Expr& eval(Env& env) override;
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
    auto mul = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        auto result = std::accumulate(args.begin(), args.end(), 1.0, [](auto p, std::shared_ptr<Expr> const& arg)
        {
            auto v = dynamic_cast<Number&>(*arg);
            switch (v.mNumber.index())
            {
            case 0:
                return p * std::get<0>(v.mNumber);
            
            case 1:
                return p * std::get<1>(v.mNumber);
            }
            return 1.0;
        }
        );
        return std::shared_ptr<Expr>(new Number(result)); 
    };
    auto a = std::shared_ptr<Expr>(new Number(5));
    auto b = std::shared_ptr<Expr>(new Number(0.5));
    auto mulOp = PrimitiveProcedure{mul};
    std::cout << mulOp.apply({a, b})->toString() << std::endl;
    return 0;
}