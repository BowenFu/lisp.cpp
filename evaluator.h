#ifndef LISP_EVALUATOR_H
#define LISP_EVALUATOR_H

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
    std::map<std::string, ExprPtr> mFrame;
    Env* mEnclosingEnvironment;
public:
    Env() = default;
    Env(std::map<std::string, ExprPtr> frame, Env* enclosingEnvironment)
    : mFrame{frame}
    , mEnclosingEnvironment{enclosingEnvironment}
    {}
    ExprPtr lookupVariableValue(std::string const& variableName)
    {
        Env* env = this;
        while (env)
        {
            auto iter = env->mFrame.find(variableName);
            if (iter != env->mFrame.end())
            {
                return iter->second;
            }
            env = env->mEnclosingEnvironment;
        }
        // assert(false);
        throw std::runtime_error{variableName};
    }
    ExprPtr setVariableValue(std::string const& variableName, ExprPtr value)
    {
        auto iter = mFrame.find(variableName);
        assert(iter != mFrame.end());
        iter->second = value;
        return value;
    }
    ExprPtr defineVariable(std::string const& variableName, ExprPtr value)
    {
        auto iter = mFrame.find(variableName);
        assert(iter == mFrame.end());
        mFrame.insert({variableName, value});
        return value;
    }
    std::shared_ptr<Env> extend(std::vector<std::string> const& parameters, std::vector<ExprPtr> const& arguments)
    {
        assert(parameters.size() == arguments.size());
        std::map<std::string, ExprPtr> frame;
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
    std::string mName;
public:
    Variable(std::string const& name)
    : mName{name}
    {}
    ExprPtr eval(Env& env) override
    {
        return env.lookupVariableValue(mName);
    }
    std::string name() const
    {
        return mName;
    }
    std::string toString() const override
    {
        return mName;
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
    std::shared_ptr<Expr> mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Assignment(std::shared_ptr<Expr> var, std::shared_ptr<Expr> value)
    : mVariable{var}
    , mValue{value}
    {
    }
    ExprPtr eval(Env& env) override
    {
        return env.setVariableValue(dynamic_cast<Variable*>(mVariable.get())->name(), mValue->eval(env));
    }
    std::string toString() const override
    {
        return "Assignment";
    }
};

class Definition final : public Expr
{
    std::shared_ptr<Expr> mVariable;
    std::shared_ptr<Expr> mValue;
public:
    Definition(std::shared_ptr<Expr> var, std::shared_ptr<Expr> value)
    : mVariable{var}
    , mValue{value}
    {
    }
    ExprPtr eval(Env& env) override
    {
        return env.defineVariable(dynamic_cast<Variable*>(mVariable.get())->name(), mValue->eval(env));
    }
    std::string toString() const override
    {
        return "Definition  ( " + mVariable->toString() + " : " + mValue->toString() + " )";
    }
};

inline bool isTrue(ExprPtr expr)
{
    using T = Literal<bool>;
    T const* v = dynamic_cast<T const*>(expr.get());
    return v != nullptr && v->get();
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

inline std::vector<ExprPtr> listOfValues(std::vector<ExprPtr> const& exprs, Env& env)
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
    std::vector<std::string> mParameters;
    std::shared_ptr<Env> mEnvironment;
public:
    CompoundProcedure(std::shared_ptr<Sequence> body, std::vector<std::string> parameters, std::shared_ptr<Env> environment)
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

#endif // LISP_EVALUATOR_H