#include <cstdlib>
#include <memory>

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
    virtual ~Expr() = default;
};

class Literal final : public Expr
{
public:
    Expr& eval(Env& env) override
    {
        return *this;
    }
};

class Variable final : public Expr
{
public:
    Expr& eval(Env& env) override
    {
        return env.lookupVariableValue(*this);
    }
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
};

class Lambda final : public Expr
{
public:
    Expr& eval(Env& env) override;
};

class Sequence final : public Expr
{
public:
    Expr& eval(Env& env) override;
};

class Cond final : public Expr
{
public:
    Expr& eval(Env& env) override;
};

class Application final : public Expr
{
public:
    Expr& eval(Env& env) override;
};

int32_t main()
{
    return 0;
}