#ifndef LISP_PARSER_H
#define LISP_PARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/evaluator.h"
#include "lisp/lexer.h"
#include "lisp/metaParser.h"
#include <cctype>
#include <optional>

inline auto parse(MExprPtr const& mexpr) -> ExprPtr;

inline auto deCons(MExprPtr const& mexpr)
{
    auto cons = dynamic_cast<MCons*>(mexpr.get());
    ASSERT(cons);
    return std::make_pair(cons->car(), cons->cdr());
}

inline std::optional<std::string> asString(MExprPtr const& mexpr)
{
    auto atomic = dynamic_cast<MAtomic*>(mexpr.get());
    if (atomic)
    {
        return atomic->get();
    }
    return {};
}

inline std::vector<std::string> parseParams(MExprPtr const& mexpr)
{
    std::vector<std::string> params;
    auto me = mexpr;
    while (me != MNil::instance())
    {
        auto [car, cdr] = deCons(me);
        auto opStr = asString(car);
        ASSERT(opStr.has_value());
        params.push_back(opStr.value());
        me = cdr;
    }
    return params;
}

inline MExprPtr listBack(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    ASSERT(cdr == MNil::instance());
    return car;
}

inline std::vector<ExprPtr> parseActions(MExprPtr const& mexpr)
{
    std::vector<ExprPtr> actions;
    auto me = mexpr;
    while (me != MNil::instance())
    {
        auto [car, cdr] = deCons(me);
        actions.push_back(parse(car));
        me = cdr;
    }
    return actions;
}

inline std::shared_ptr<Sequence> sequence(MExprPtr const& mexpr)
{
    return std::make_shared<Sequence>(parseActions(mexpr));
}

inline ExprPtr and_(MExprPtr const& mexpr)
{
    return std::make_shared<And>(parseActions(mexpr));
}

inline ExprPtr or_(MExprPtr const& mexpr)
{
    return std::make_shared<Or>(parseActions(mexpr));
}

inline ExprPtr definition(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
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

inline ExprPtr assignment(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    auto var = asString(car).value();
    auto value = parse(listBack(cdr));
    return ExprPtr{new Assignment(var, value)};
}

inline ExprPtr lambda(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    auto params = parseParams(car);
    auto body = sequence(cdr);
    return ExprPtr{new Lambda(params, body)};
}

inline ExprPtr if_(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    auto predicate = parse(car);
    auto [cdrA, cdrD] = deCons(cdr);
    auto consequent = parse(cdrA);
    auto alternative = parse(listBack(cdrD));
    return ExprPtr{new If(predicate, consequent, alternative)};
}

inline std::pair<ExprPtr, ExprPtr> parseCondClauses(MExprPtr const& mexpr, bool& hasNext)
{
    auto [car, cdr] = deCons(mexpr);
    
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

inline ExprPtr cond(MExprPtr const& mexpr)
{
    std::vector<std::pair<ExprPtr, ExprPtr>> condClauses;
    bool hasNext = true;
    auto me = mexpr;
    while (hasNext && me != MNil::instance())
    {
        auto [car, cdr] = deCons(me);
        condClauses.push_back(parseCondClauses(car, hasNext));
        me = cdr;
    }
    return ExprPtr{new Cond(condClauses)};
}

inline ExprPtr application(MExprPtr const& car, MExprPtr const& cdr)
{
    auto op = parse(car);
    std::vector<ExprPtr> params = parseActions(cdr);
    return ExprPtr{new Application(op, params)};
}

inline ExprPtr application(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    return application(car, cdr);
}

inline auto tryMAtomic(MExprPtr const& mexpr) -> ExprPtr
{
    auto opStr = asString(mexpr);
    if (!opStr.has_value())
    {
        return {};
    }
    auto str = opStr.value();
    auto c = str.front();
    if (isdigit(c) || (str.size() > 1 && c == '-'))
    {
        double num = std::stod(str);
        return ExprPtr{new Number(num)};
    }
    if (c == '"')
    {
        auto substr = str.substr(1U, str.size() - 2U);
        return ExprPtr{new String(substr)};
    }
    return ExprPtr{new Variable(str)};
}

inline auto tryMCons(MExprPtr const& mexpr) -> ExprPtr
{
    auto cons = dynamic_cast<MCons*>(mexpr.get());
    if (!cons)
    {
        return {};
    }
    auto car = cons->car();
    auto cdr = cons->cdr();
    auto carStr = asString(car);
    if (!carStr.has_value())
    {
        // as MCons
        return application(car, cdr);
    }
    if (carStr == "define")
    {
        return definition(cdr);
    }
    else if (carStr == "set!")
    {
        return assignment(cdr);
    }
    else if (carStr == "lambda")
    {
        return lambda(cdr);
    }
    else if (carStr == "if")
    {
        return if_(cdr);
    }
    else if (carStr == "cond")
    {
        return cond(cdr);
    }
    else if (carStr == "begin")
    {
        return std::static_pointer_cast<Expr>(sequence(cdr));
    }
    else if (carStr == "and")
    {
        return and_(cdr);
    }
    else if (carStr == "or")
    {
        return or_(cdr);
    }
    return application(car, cdr);
}

inline auto parse(MExprPtr const& mexpr) -> ExprPtr
{
    if (auto e = tryMAtomic(mexpr))
    {
        return e;
    }
    if (auto e = tryMCons(mexpr))
    {
        return e;
    }
    FAIL("Not implemented!");
    return {};
}

#endif // LISP_PARSER_H