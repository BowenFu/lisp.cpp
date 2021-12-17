#ifndef LISP_EVALUATOR_H
#define LISP_EVALUATOR_H

#include "meta.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>

class Expr;
using ExprPtr = std::shared_ptr<Expr>;
class Env;

class Variable;

template <typename Iter>
ExprPtr vecToCons(Iter begin, Iter end);

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
    void clear()
    {
        mFrame.clear();
        mEnclosingEnvironment = nullptr;
    }
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
        throw std::runtime_error{"variable " + variableName + " not found!"};
    }
    ExprPtr setVariableValue(std::string const& variableName, ExprPtr value)
    {
        auto iter = mFrame.find(variableName);
        if (iter == mFrame.end())
        {
            throw std::runtime_error{"call setVariableValue to undefined variables." + variableName};
        }
        iter->second = value;
        return value;
    }
    bool variableDefined(std::string const& variableName)
    {
        if (mFrame.count(variableName))
        {
            return true;
        }
        return false;
    }
    ExprPtr defineVariable(std::string const& variableName, ExprPtr value)
    {
        if (mFrame.count(variableName))
        {
            throw std::runtime_error{"call defineVariable to defined variables." + variableName};
        }
        mFrame.insert({variableName, value});
        return value;
    }
    std::shared_ptr<Env> extend(std::vector<std::string> const& parameters, std::vector<ExprPtr> const& arguments, bool variadic = false)
    {
        if (variadic)
        {
            ASSERT(parameters.size() <= arguments.size());
        }
        else
        {
            ASSERT(parameters.size() == arguments.size());
        }
        std::map<std::string, ExprPtr> frame;
        if (!parameters.empty())
        {
            for (size_t i = 0; i < parameters.size() - 1; ++i)
            {
                frame.insert({parameters.at(i), arguments.at(i)});
            }
            if (variadic)
            {
                frame.insert({parameters.back(), vecToCons(arguments.rbegin(), arguments.rend() - static_cast<long>(parameters.size()) + 1)});
            }
            else
            {
                frame.insert({parameters.back(), arguments.back()});
            }
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

template <>
inline std::string Literal<std::string>::toString() const
{
    return "\"" + mValue + "\"";
}

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
        return "()";
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
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return ExprPtr{new Cons{mCar->eval(env), mCdr->eval(env)}};
    }
    std::string toString() const override
    {
        std::ostringstream o;
        o << "(" << mCar->toString();
        if (dynamic_cast<Cons*>(mCdr.get()))
        {
            auto cdrStr = mCdr->toString();
            auto cdrStrSize = cdrStr.size();
            o << " " << cdrStr.substr(1U, cdrStrSize - 2);
        }
        else if (mCdr == nil())
        {
        }
        else
        {
            o << " . " << mCdr->toString();
        }
        o << ")";
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

template <typename Iter>
ExprPtr vecToCons(Iter begin, Iter end)
{
    auto result = nil();
    for (auto i = begin; i != end; ++i)
    {
        result = ExprPtr{new Cons{*i, result}};
    }
    return result;
}

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

class Symbol final : public Expr, public std::enable_shared_from_this<Symbol>
{
    std::string mInternal;
public:
    explicit Symbol(std::string const& name)
    : mInternal{name}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return shared_from_this();
    }
    std::string toString() const override
    {
        return "'" + mInternal;
    }
    std::string get() const
    {
        return mInternal;
    }
};

// For meta parser only
class RawWord final : public Expr, public std::enable_shared_from_this<RawWord>
{
    std::string mInternal;
public:
    explicit RawWord(std::string const& name)
    : mInternal{name}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return shared_from_this();
    }
    std::string toString() const override
    {
        return mInternal;
    }
    std::string get() const
    {
        return mInternal;
    }
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

class Macro;

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
    bool isMacroDefinition() const;
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
    return v == nullptr || v->get();
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
        return "Or";
    }
};

template <typename ProcedureT>
class LambdaBase : public Expr
{
    std::vector<std::string> mParameters;
    bool mVariadic;
    std::shared_ptr<Sequence> mBody;
public:
    LambdaBase(std::vector<std::string> const& params, bool variadic, std::shared_ptr<Sequence> body)
    : mParameters{params}
    , mVariadic{variadic}
    , mBody{body}
    {
    }
    ExprPtr eval(std::shared_ptr<Env> const& env) override
    {
        return std::shared_ptr<Expr>{new ProcedureT{mBody, mParameters, mVariadic, env}};
    }
};

class CompoundProcedure;

class Lambda final : public LambdaBase<CompoundProcedure>
{
public:
    using LambdaBase::LambdaBase;
    std::string toString() const override
    {
        return "Lambda";
    }
};

class MacroProcedure;

class Macro final : public LambdaBase<MacroProcedure>
{
public:
    using LambdaBase::LambdaBase;
    std::string toString() const override
    {
        return "Macro";
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

class CompoundProcedureBase : public Procedure, public std::enable_shared_from_this<CompoundProcedureBase>
{
    std::shared_ptr<Sequence> mBody;
    std::vector<std::string> mParameters;
    bool mVariadic;
    std::shared_ptr<Env> mEnvironment;
    virtual std::string getClassName() const = 0;
public:
    CompoundProcedureBase(std::shared_ptr<Sequence> body, std::vector<std::string> parameters, bool variadic, std::shared_ptr<Env> const& environment)
    : mBody{body}
    , mParameters{parameters}
    , mVariadic{variadic}
    , mEnvironment{environment}
    {}
    ExprPtr eval(std::shared_ptr<Env> const& /* env */) override
    {
        return shared_from_this();
    }
    std::shared_ptr<Expr> apply(std::vector<std::shared_ptr<Expr>> const& args) override
    {
        return mBody->eval(mEnvironment->extend(mParameters, args, mVariadic));
    }
    std::string toString() const override
    {
            std::ostringstream o;
            o << getClassName() << " (";
            if (!mParameters.empty())
            {
                for (auto i = mParameters.begin(); i != std::prev(mParameters.end()); ++i)
                {
                    o << *i << " ";
                }
                if (mVariadic)
                {
                    o << ". ";
                }
                if (mParameters.size() >= 1)
                {
                    o << mParameters.back();
                }
                o << ", ";
            }
            o << "<procedure-env>)";
            return o.str();
        }
};

class CompoundProcedure final : public CompoundProcedureBase
{
    std::string getClassName() const override
    {
        return "CompoundProcedure";
    }
public:
    using CompoundProcedureBase::CompoundProcedureBase;
};

template <typename Func>
ExprPtr transform(ExprPtr const& expr, Func func)
{
    if (auto e = dynamic_cast<Cons const*>(expr.get()))
    {
        return ExprPtr{new Cons{transform(e->car(), func), transform(e->cdr(), func)}};
    }
    return func(expr);
}

class MacroProcedure final : public CompoundProcedureBase
{
    std::string getClassName() const override
    {
        return "MacroProcedure";
    }
public:
    using CompoundProcedureBase::CompoundProcedureBase;
    ExprPtr apply(std::vector<std::shared_ptr<Expr> > const &args) override;
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
        auto isMacroCall = dynamic_cast<MacroProcedure const*>(op.get());
        auto args = isMacroCall ? mOperands : listOfValues(mOperands, env);
        return dynamic_cast<Procedure&>(*op).apply(args);
    }
    std::string toString() const override
    {
        std::ostringstream o;
        o << "Application (" << mOperator->toString();
        for (auto const& e: mOperands)
        {
            o << " " << e->toString();
        }
        o << ")";
        return o.str();
    }
};

#endif // LISP_EVALUATOR_H
