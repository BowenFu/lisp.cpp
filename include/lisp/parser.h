#ifndef LISP_PARSER_H
#define LISP_PARSER_H

#include <string>
#include <sstream>
#include "lisp/evaluator.h"
#include "lisp/lexer.h"
#include "lisp/metaParser.h"
#include <cctype>
#include <optional>
#include <functional>

inline auto parse(ExprPtr const& expr) -> ExprPtr;

inline auto deCons(ExprPtr const& expr)
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    ASSERT(cons);
    return std::make_pair(cons->car(), cons->cdr());
}

inline auto tryDeCons(ExprPtr const& expr) -> std::optional<std::pair<ExprPtr, ExprPtr>>
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    if (cons)
    {
        return std::make_optional(std::make_pair(cons->car(), cons->cdr()));
    }
    return {};
}

inline std::optional<std::string> asString(ExprPtr const& expr)
{
    auto atomic = dynamic_cast<RawWord*>(expr.get());
    if (atomic)
    {
        return atomic->toString();
    }
    return {};
}

inline Params parseParams(ExprPtr const& expr)
{
    std::vector<std::string> params;
    auto me = expr;
    if (auto e = dynamic_cast<RawWord*>(me.get()))
    {
        return e->get();
    }
    while (me != nil())
    {
        auto cons = dynamic_cast<Cons*>(me.get());
        if (!cons)
        {
            auto opStr = asString(me);
            ASSERT(opStr.has_value());
            params.push_back(opStr.value());
            return std::make_pair(params, true);
        }
        auto opStr = asString(cons->car());
        ASSERT(opStr.has_value());
        params.push_back(opStr.value());
        me = cons->cdr();
    }
    return std::make_pair(params, false);
}

inline ExprPtr listBack(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    ASSERT(cdr == nil());
    return car;
}

inline std::vector<ExprPtr> consToVec(ExprPtr const& expr)
{
    std::vector<ExprPtr> vec;
    auto me = expr;
    while (me != nil())
    {
        auto [car, cdr] = deCons(me);
        vec.push_back(car);
        me = cdr;
    }
    return vec;
}

inline std::vector<ExprPtr> parseActions(ExprPtr const& expr)
{
    std::vector<ExprPtr> actions;
    auto me = expr;
    while (me != nil())
    {
        auto [car, cdr] = deCons(me);
        actions.push_back(parse(car));
        me = cdr;
    }
    return actions;
}

inline std::shared_ptr<Sequence> sequence(ExprPtr const& expr)
{
    return std::make_shared<Sequence>(parseActions(expr));
}

inline ExprPtr and_(ExprPtr const& expr)
{
    return std::make_shared<And>(parseActions(expr));
}

inline ExprPtr or_(ExprPtr const& expr)
{
    return std::make_shared<Or>(parseActions(expr));
}

inline auto parseAsQuoted(ExprPtr const& expr, std::optional<int32_t> quasiquoteLevel) -> ExprPtr;

inline ExprPtr assertIsLastAndGet(ExprPtr const& expr)
{
    auto e = dynamic_cast<Cons*>(expr.get());
    ASSERT(e);
    auto car = e->car();
    ASSERT(car);
    // assert expr is the last one
    auto cdr = e->cdr();
    ASSERT(cdr == nil());
    return car;
}

inline ExprPtr quote(ExprPtr const& expr)
{
    return parseAsQuoted(assertIsLastAndGet(expr), /* quasiquoteLevel = */ {});
}

inline ExprPtr quasiquote(ExprPtr const& expr)
{
    return parseAsQuoted(assertIsLastAndGet(expr), /* quasiquoteLevel = */ 1);
}

inline ExprPtr definition(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    auto opStr = asString(car);
    if (!opStr.has_value())
    {
        auto [carA, carD] = deCons(car);
        auto opStr = asString(carA);
        ASSERT(opStr.has_value());
        auto params = parseParams(carD);
        auto body = sequence(cdr);
        auto proc = ExprPtr{new Lambda(params, body)};
        return ExprPtr{new Definition(opStr.value(), proc)};
    }
    // normal definition
    auto value = parse(listBack(cdr));
    return ExprPtr{new Definition(opStr.value(), value)};
}

inline ExprPtr assignment(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    auto var = asString(car).value();
    auto value = parse(listBack(cdr));
    return ExprPtr{new Assignment(var, value)};
}

template <typename LambdaT>
inline ExprPtr lambdaBase(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    auto params = parseParams(car);
    auto body = sequence(cdr);
    return ExprPtr{new LambdaT(params, body)};
}

inline ExprPtr lambda(ExprPtr const& expr)
{
    return lambdaBase<Lambda>(expr);
}

inline ExprPtr macro(ExprPtr const& expr)
{
    return lambdaBase<Macro>(expr);
}

inline ExprPtr if_(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    auto predicate = parse(car);
    auto [cdrA, cdrD] = deCons(cdr);
    auto consequent = parse(cdrA);
    auto alternative = parse(listBack(cdrD));
    return ExprPtr{new If(predicate, consequent, alternative)};
}

inline std::pair<ExprPtr, ExprPtr> parseCondClauses(ExprPtr const& expr, bool& hasNext)
{
    auto [car, cdr] = deCons(expr);
    
    ExprPtr pred;
    auto opStr = asString(car);
    if (opStr && opStr.value() == "else")
    {
        pred = true_();
        hasNext = false;
    }
    else
    {
        pred = parse(car);
    }
    auto action = parse(listBack(cdr));
    return {pred, action};
}

inline ExprPtr cond(ExprPtr const& expr)
{
    std::vector<std::pair<ExprPtr, ExprPtr>> condClauses;
    bool hasNext = true;
    auto me = expr;
    while (hasNext && me != nil())
    {
        auto [car, cdr] = deCons(me);
        condClauses.push_back(parseCondClauses(car, hasNext));
        me = cdr;
    }
    return ExprPtr{new Cond(condClauses)};
}

inline ExprPtr application(ExprPtr const& car, ExprPtr const& cdr)
{
    auto op = parse(car);
    std::vector<ExprPtr> params = parseActions(cdr);
    return ExprPtr{new Application(op, params)};
}

inline ExprPtr tryMacroApplication(ExprPtr const& car, ExprPtr const& cdr, std::shared_ptr<Env> const& env)
{
    auto op = parse(car);
    auto evaledOp = op->eval(env);
    if (dynamic_cast<MacroProcedure const*>(evaledOp.get()))
    {
        std::vector<ExprPtr> params = consToVec(cdr);
        return Application(evaledOp, params).eval(env);
    }
    return ExprPtr{new Cons{car, cdr}};
}

inline ExprPtr application(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    return application(car, cdr);
}

inline auto parseCons(std::string const& car, ExprPtr const& cdr, std::map<std::string, std::function<ExprPtr(ExprPtr)>> const& keywordToHandler) -> ExprPtr
{
    auto iter = keywordToHandler.find(car);
    if (iter != keywordToHandler.end())
    {
        return iter->second(cdr);
    }
    return keywordToHandler.at("_")(cdr);
}

inline auto tryCons(ExprPtr const& expr) -> ExprPtr
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    if (!cons)
    {
        return {};
    }
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = asString(car);
    if (!carStr.has_value())
    {
        // car as Cons
        return application(car, cdr);
    }
    std::map<std::string, std::function<ExprPtr(ExprPtr)>> keywordToHandler;
    keywordToHandler["define"] = definition;
    keywordToHandler["set!"] = assignment;
    keywordToHandler["lambda"] = lambda;
    keywordToHandler["macro"] = macro;
    keywordToHandler["if"] = if_;
    keywordToHandler["cond"] = cond;
    keywordToHandler["begin"] = [](auto cdr){ return std::static_pointer_cast<Expr>(sequence(cdr));};
    keywordToHandler["and"] = and_;
    keywordToHandler["or"] = or_;
    keywordToHandler["quote"] = quote;
    keywordToHandler["quasiquote"] = quasiquote;
    keywordToHandler["_"] = [car](auto cdr){ return application(car, cdr); };
    return parseCons(carStr.value(), cdr, keywordToHandler);
}

inline auto tryParseMacroDefinitionBody(ExprPtr const& expr) -> ExprPtr
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    if (!cons)
    {
        return {};
    }
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = asString(car);
    if (!carStr.has_value())
    {
        // do nothing
        return {};
    }
    std::map<std::string, std::function<ExprPtr(ExprPtr)>> keywordToHandler;
    keywordToHandler["macro"] = macro;
    keywordToHandler["_"] = [expr](auto){ return ExprPtr{}; };
    return parseCons(carStr.value(), cdr, keywordToHandler);
}

inline ExprPtr macroDefinition(ExprPtr const& expr)
{
    auto [car, cdr] = deCons(expr);
    auto opStr = asString(car);
    if (!opStr)
    {
        return {};
    }
    // normal definition
    if (auto value = tryParseMacroDefinitionBody(listBack(cdr)))
    {
        return ExprPtr{new Definition(opStr.value(), value)};
    }
    return {};
}

inline auto parseMacroDefinition(ExprPtr const& expr) -> ExprPtr
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    if (!cons)
    {
        return {};
    }
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = asString(car);
    if (!carStr.has_value())
    {
        // do nothing
        return {};
    }
    std::map<std::string, std::function<ExprPtr(ExprPtr)>> keywordToHandler;
    keywordToHandler["define"] = macroDefinition;
    keywordToHandler["_"] = [expr](auto){ return ExprPtr{}; };
    return parseCons(carStr.value(), cdr, keywordToHandler);
}

inline auto tryMacroCall(ExprPtr const& expr, std::shared_ptr<Env> const& env) -> ExprPtr
{
    auto cons = dynamic_cast<Cons*>(expr.get());
    if (!cons)
    {
        return expr;
    }
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = asString(car);
    if (!carStr.has_value())
    {
        // do nothing
        return expr;
    }
    if (env->variableDefined(carStr.value()))
    {
        return tryMacroApplication(car, cdr, env);
    }
    return expr;
}

inline auto parse(ExprPtr const& expr) -> ExprPtr
{
    if (auto e = tryCons(expr))
    {
        return e;
    }
    if (auto e = dynamic_cast<RawWord const*>(expr.get()))
    {
        return ExprPtr{new Variable{e->toString()}};
    }
    return expr;
}

inline auto atomicToQuoted(ExprPtr const& expr)
{
    if (auto word = dynamic_cast<RawWord const*>(expr.get()))
    {
        return ExprPtr{new Symbol{word->get()}};
    }
    if (dynamic_cast<Variable const*>(expr.get()))
    {
        FAIL("Variable should not appear as atomic!");
    }
    if (dynamic_cast<Symbol const*>(expr.get()))
    {
        FAIL("Symbol should not appear as atomic!");
    }
    return expr;
}

inline ExprPtr unquote(ExprPtr const& expr)
{
    return parse(assertIsLastAndGet(expr));
}

inline auto consToQuoted(ExprPtr const& expr, std::optional<int32_t> quasiquoteLevel) -> ExprPtr
{
    if (expr == nil())
    {
        return nil();
    }
    auto cons = dynamic_cast<Cons*>(expr.get());
    ASSERT(cons);
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = car->toString();
    if (quasiquoteLevel)
    {
        if (carStr == "unquote" || carStr == "unquote-splicing")
        {
            if ( quasiquoteLevel.value() == 1)
            {
                auto result = unquote(cdr);
                if (carStr == "unquote")
                {
                    return result;
                }
                return result;
            }
            --(*quasiquoteLevel);
        }
        else if (carStr == "quasiquote")
        {
            ++(*quasiquoteLevel);
        }
    }
    return ExprPtr{new Cons{parseAsQuoted(car, quasiquoteLevel), consToQuoted(cdr, quasiquoteLevel)}};
}

inline auto parseAsQuoted(ExprPtr const& expr, std::optional<int32_t> quasiquoteLevel) -> ExprPtr
{
    if (!dynamic_cast<Cons*>(expr.get()))
    {
        return atomicToQuoted(expr);
    }

    // else expr is cons, map quote on it
    return consToQuoted(expr, quasiquoteLevel);
}

// FIXME
inline auto expandMacros(ExprPtr const& expr, std::shared_ptr<Env> const& env) -> ExprPtr
{
    if (auto macroDefinition = parseMacroDefinition(expr))
    {
        macroDefinition->eval(env);
        return nil();
    }
    if (auto e = tryMacroCall(expr, env))
    {
        return e;
    }
    if (auto c = dynamic_cast<Cons const*>(expr.get()))
    {
        return ExprPtr{new Cons{expandMacros(c->car(), env), expandMacros(c->cdr(), env)}};
    }
    return expr;
}

#endif // LISP_PARSER_H
