#ifndef LISP_EVALUATOR_H
#define LISP_EVALUATOR_H

#include "lisp/meta.h"

class Expr;
using ExprPtr = std::shared_ptr<Expr>;
class Env;

class Variable;

class Env
{
    std::map<std::string, ExprPtr> mFrame;
    Env* mEnclosingEnvironment;
public:
    Env()
    : mFrame{}
    , mEnclosingEnvironment{nullptr}
    {}
    Env(std::map<std::string, ExprPtr> frame, Env* enclosingEnvironment)
    : mFrame{frame}
    , mEnclosingEnvironment{enclosingEnvironment}
    {}
    ExprPtr lookupVariableValue(std::string const& variableName)
    {
        Env* env = this;
        while (env != nullptr)
        {
            auto iter = env->mFrame.find(variableName);
            if (iter != env->mFrame.end())
            {
                return iter->second;
            }
            env = env->mEnclosingEnvironment;
        }
        throw std::runtime_error{variableName + " not found!"};
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
    virtual bool equalTo(ExprPtr const&) const
    {
        FAIL("Not implemented");
    }
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
        o << std::boolalpha << mValue;
        return o.str();
    }
    Value get() const
    {
        return mValue;
    }
    bool equalTo(ExprPtr const& other) const override
    {
        auto theOther = dynamic_cast<Literal*>(other.get());
        if (theOther)
        {
            return get() == theOther->get();
        }
        return false;
    }
};

using Number = Literal<double>;
using String = Literal<std::string>;
using Bool = Literal<bool>;

ExprPtr true_();
ExprPtr false_();

ExprPtr nil();

class Nil final: public Expr
{
public:
    Nil() = default;
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return nil();
    }
    std::string toString() const override
    {
        return "nil";
    }
    bool equalTo(ExprPtr const& other) const override
    {
        return this == other.get();
    }
};

class Cons final: public Expr
{
    ExprPtr mCar;
    ExprPtr mCdr;
public:
    Cons(ExprPtr const& car_, ExprPtr const& cdr_)
    : mCar{car_}
    , mCdr{cdr_}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return ExprPtr{new Cons(*this)};
    }
    std::string toString() const override
    {
        std::ostringstream o;
        o << "Cons (" << mCar->toString() << ", " << mCdr->toString() << ")";
        return o.str();
    }
    auto car() const
    {
        return mCar;
    }
    auto cdr() const
    {
        return mCdr;
    }
    bool equalTo(ExprPtr const& other) const override
    {
        auto theOther = dynamic_cast<Cons*>(other.get());
        if (theOther)
        {
            return car()->equalTo(theOther->car()) && cdr()->equalTo(theOther->cdr());
        }
        return false;
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
    std::string mVariableName;
    std::shared_ptr<Expr> mValue;
public:
    Assignment(std::string const& varName, std::shared_ptr<Expr> value)
    : mVariableName{varName}
    , mValue{value}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return env->setVariableValue(mVariableName, mValue->eval(env));
    }
    std::string toString() const override
    {
        return "Assignment ( " + mVariableName + " : " + mValue->toString() + " )";
    }
};

class Definition final : public Expr
{
    std::string mVariableName;
    std::shared_ptr<Expr> mValue;
public:
    Definition(std::string const& varName, std::shared_ptr<Expr> value)
    : mVariableName{varName}
    , mValue{value}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& env) override;
    std::string toString() const override
    {
        return "Definition ( " + mVariableName + " : " + mValue->toString() + " )";
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

class And final : public Expr
{
    std::vector<ExprPtr> mActions;
public:
    And(std::vector<ExprPtr> actions)
    : mActions{actions}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        bool const result = std::all_of(mActions.begin(), mActions.end(), [&env](auto& e){ return isTrue(e->eval(env)); });
        return result ? true_() : false_();
    }
    std::string toString() const override
    {
        return "And";
    }
};

class Or final : public Expr
{
    std::vector<ExprPtr> mActions;
public:
    Or(std::vector<ExprPtr> actions)
    : mActions{actions}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        bool const result = std::any_of(mActions.begin(), mActions.end(), [&env](auto& e){ return isTrue(e->eval(env)); });
        return result ? true_() : false_();
    }
    std::string toString() const override
    {
        return "Any";
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
    std::vector<std::pair<ExprPtr, ExprPtr>> mClauses;
public:
    Cond(std::vector<std::pair<ExprPtr, ExprPtr>> const &clauses)
        : mClauses{clauses}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        for (auto& e : mClauses)
        {
            if (isTrue(e.first->eval(env)))
            {
                return e.second->eval(env);
            }
        }
        FAIL("Missing case in cond!");
    }
    std::string toString() const override
    {
        return "Cond";
    }
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
        std::ostringstream o;
        o << "CompoundProcedure (";
        for (auto const& p : mParameters)
        {
            o << p << ", ";
        }
        o << "<procedure-env>)";
        return o.str();
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
