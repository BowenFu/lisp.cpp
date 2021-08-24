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
#include <algorithm>
#include <functional>

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
        throw std::runtime_error{variableName};
    }
    ExprPtr setVariableValue(std::string const& variableName, ExprPtr value)
    {
        auto iter = mFrame.find(variableName);
        if (iter == mFrame.end())
        {
            throw std::runtime_error{"call setVariableValue to undefined variables."};
        }
        iter->second = value;
        return value;
    }
    ExprPtr defineVariable(std::string const& variableName, ExprPtr value)
    {
        auto iter = mFrame.find(variableName);
        if (iter != mFrame.end())
        {
            throw std::runtime_error{"call defineVariable to defined variables."};
        }
        mFrame.insert({variableName, value});
        return value;
    }
    std::shared_ptr<Env> extend(std::vector<std::string> const& parameters, std::vector<ExprPtr> const& arguments)
    {
        if (parameters.size() != arguments.size())
        {
            throw std::runtime_error{"parameters unmatched"};
        }
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
    virtual ExprPtr eval(std::shared_ptr<Env> const& env) = 0;
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
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
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
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return env->lookupVariableValue(mName);
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
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
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
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return env->setVariableValue(dynamic_cast<Variable*>(mVariable.get())->name(), mValue->eval(env));
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
    ExprPtr eval(std::shared_ptr<Env> const& env) override;
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
    If(ExprPtr const& predicate, ExprPtr const& consequent, ExprPtr const& alternative)
    : mPredicate{predicate}
    , mConsequent{consequent}
    , mAlternative{alternative}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return isTrue(mPredicate->eval(env)) ? mConsequent->eval(env) : mAlternative->eval(env);
    }
    std::string toString() const override
    {
        return "(if " + mPredicate->toString() + " " + mConsequent->toString() + " " + mAlternative->toString() + ")";
    }
};

class Sequence final : public Expr
{
    std::vector<ExprPtr> mActions;
public:
    Sequence(std::vector<ExprPtr> actions)
    : mActions{actions}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        for (size_t i = 0; i < mActions.size() - 1; i++)
        {
            mActions.at(i)->eval(env);
        }
        return mActions.back()->eval(env);
    }
    std::string toString() const override
    {
        return "Sequence";
    }
};

class Lambda final : public Expr
{
    std::vector<std::string> mParameters;
    std::shared_ptr<Sequence> mBody;
public:
    Lambda(std::vector<std::string> const& params, std::shared_ptr<Sequence> body)
    : mParameters{params}
    , mBody{body}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& env) override;
    std::string toString() const override
    {
        return "Lambda";
    }
};

class Cond final : public Expr
{
    std::shared_ptr<Expr> mClauses;
public:
    ExprPtr eval(std::shared_ptr<Env> const& env) override;
    std::string toString() const override;
};

inline std::vector<ExprPtr> listOfValues(std::vector<ExprPtr> const& exprs, std::shared_ptr<Env> const& env)
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
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
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
    std::weak_ptr<Env> mEnvironment;
public:
    CompoundProcedure(std::shared_ptr<Sequence> body, std::vector<std::string> parameters, std::shared_ptr<Env> const& environment)
    : mBody{body}
    , mParameters{parameters}
    , mEnvironment{environment}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return ExprPtr{new CompoundProcedure{mBody, mParameters, std::shared_ptr<Env>{mEnvironment}}};
    }
    std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) override
    {
        return mBody->eval(std::shared_ptr<Env>{mEnvironment}->extend(mParameters, args));
    }
    std::string toString() const override
    {
        return "CompoundProcedure";
    }
};

class Application final : public Expr
{
    ExprPtr mOperator;
    std::vector<ExprPtr> mOperands;
public:
    Application(ExprPtr const& op, std::vector<ExprPtr> const& params)
    : mOperator{op}
    , mOperands{params}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        auto op = mOperator->eval(env);
        auto args = listOfValues(mOperands, env);
        return dynamic_cast<Procedure&>(*op).apply(args);
    }
    std::string toString() const override
    {
        return "Application (" + mOperator->toString() + ")";
    }
};

#endif // LISP_EVALUATOR_H
