#ifndef LISP_PARSER_H
#define LISP_PARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/evaluator.h"
#include "lisp/lexer.h"
#include "lisp/metaParser.h"
#include <cctype>

class Parser
{
public:
    Parser(Lexer const& input)
    : mInput{input}
    , mLookAhead{mInput.nextToken()}
    {}
    void consume()
    {
        mLookAhead = mInput.nextToken();
    }
    bool match(TokenType t)
    {
        if (mLookAhead.type == t)
        {
            consume();
            return true;
        }
        else
        {
            return false;
        }
    }
    bool match(Token const& token)
    {
        if (mLookAhead == token)
        {
            consume();
            return true;
        }
        else
        {
            return false;
        }
    }
    bool eof() const
    {
        return mLookAhead.type == TokenType::kEOF;
    }

    ExprPtr number()
    {
        double num = std::stod(mLookAhead.text);
        auto result = ExprPtr{new Number(num)};
        consume();
        return result;
    }
    ExprPtr string()
    {
        auto str = mLookAhead.text.substr(1U, mLookAhead.text.size() - 2U);
        auto result = ExprPtr{new String(str)};
        consume();
        return result;
    }
    ExprPtr variable()
    {
        auto result = ExprPtr{new Variable(mLookAhead.text)};
        consume();
        return result;
    }
    ExprPtr atomic()
    {
        if (mLookAhead.type != TokenType::kWORD)
        {
            throw std::runtime_error(mLookAhead.text);
        }
        auto c = mLookAhead.text.front();
        if (isdigit(c) || (mLookAhead.text.size() > 1 && c == '-'))
        {
            return number();
        }
        if (c == '"')
        {
            return string();
        }
        return variable();
    }
    ExprPtr list()
    {
        ASSERT(match(TokenType::kL_PAREN));
        auto result = listContext();
        ASSERT(result);
        ASSERT(match(TokenType::kR_PAREN));
        return result;
    }
    ExprPtr sexpr()
    {
        if (mLookAhead.type == TokenType::kL_PAREN)
        {
            return list();
        }
        return atomic();
    }
    ExprPtr listContext()
    {
        if (mLookAhead.type == TokenType::kWORD)
        {
            if (mLookAhead.text == "define")
            {
                return definition();
            }
            else if (mLookAhead.text == "set!")
            {
                return assignment();
            }
            else if (mLookAhead.text == "lambda")
            {
                return lambda();
            }
            else if (mLookAhead.text == "if")
            {
                return if_();
            }
            else if (mLookAhead.text == "cond")
            {
                return cond();
            }
            else if (mLookAhead.text == "and")
            {
                return and_();
            }
            else if (mLookAhead.text == "or")
            {
                return or_();
            }
            else if (mLookAhead.text == "begin")
            {
                consume();
                return std::static_pointer_cast<Expr>(sequence());
            }
            else
            {
                return application();
            }
        }
        if (mLookAhead.type == TokenType::kL_PAREN)
        {
            return application();
        }
        else
        {
            throw std::runtime_error("Not implemented: " + mLookAhead.text);
        }
        return {};
    }
    ExprPtr definition()
    {
        ASSERT(match({TokenType::kWORD, "define"}));
        // define procedure
        if (mLookAhead.type == TokenType::kL_PAREN)
        {
            consume();
            auto var = mLookAhead.text;
            consume();
            auto params = parseParams();
            ASSERT(match(TokenType::kR_PAREN));
            auto body = sequence();
            auto proc = ExprPtr{new Lambda(params, body)};
            return ExprPtr{new Definition(var, proc)};
        }
        // normal definition
        auto var = mLookAhead.text;
        consume();
        auto value = sexpr();
        return ExprPtr{new Definition(var, value)};
    }
    ExprPtr assignment()
    {
        ASSERT(match({TokenType::kWORD, "set!"}));
        auto var = mLookAhead.text;
        consume();
        auto value = sexpr();
        return ExprPtr{new Assignment(var, value)};
    }
    std::vector<ExprPtr> parseActions()
    {
        std::vector<ExprPtr> actions;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            actions.push_back(sexpr());
        }
        return actions;
    }
    std::shared_ptr<Sequence> sequence()
    {
        return std::make_shared<Sequence>(parseActions());
    }
    std::vector<std::string> parseParams()
    {
        std::vector<std::string> params;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            params.push_back(dynamic_cast<Variable*>(variable().get())->name());
        }
        return params;
    }
    ExprPtr lambda()
    {
        ASSERT(match({TokenType::kWORD, "lambda"}));
        ASSERT(match(TokenType::kL_PAREN));
        auto params = parseParams();
        ASSERT(match(TokenType::kR_PAREN));
        auto body = sequence();
        return ExprPtr{new Lambda(params, body)};
    }
    ExprPtr if_()
    {
        ASSERT(match({TokenType::kWORD, "if"}));
        auto predicate = sexpr();
        auto consequent = sexpr();
        auto alternative = sexpr();
        return ExprPtr{new If(predicate, consequent, alternative)};
    }
    ExprPtr cond()
    {
        ASSERT(match({TokenType::kWORD, "cond"}));
        std::vector<std::pair<ExprPtr, ExprPtr>> condClauses;
        bool hasNext = true;
        while (hasNext && mLookAhead.type != TokenType::kR_PAREN)
        {
            ASSERT(match(TokenType::kL_PAREN));
            ExprPtr pred;
            if (mLookAhead.text == "else")
            {
                pred = (consume(), true_());
                hasNext = false;
            }
            else
            {
                pred = sexpr();
            }
            auto action = sexpr();
            condClauses.emplace_back(pred, action);
            ASSERT(match(TokenType::kR_PAREN));
        }
        return ExprPtr{new Cond(condClauses)};
    }
    ExprPtr and_()
    {
        ASSERT(match({TokenType::kWORD, "and"}));
        return ExprPtr{new And(parseActions())};
    }
    ExprPtr or_()
    {
        ASSERT(match({TokenType::kWORD, "or"}));
        return ExprPtr{new And(parseActions())};
    }
    ExprPtr application()
    {
        auto op = sexpr();
        std::vector<ExprPtr> params = parseActions();
        return ExprPtr{new Application(op, params)};
    }
private:
    Lexer mInput;
    Token mLookAhead;
};

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
        auto [car, cdr] = deCons(mexpr);
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

inline ExprPtr definition(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    // define procedure
    // if (mLookAhead.type == TokenType::kL_PAREN)
    // {
    //     consume();
    //     auto var = variable();
    //     auto params = parseParams();
    //     ASSERT(match(TokenType::kR_PAREN));
    //     auto body = sequence();
    //     auto proc = ExprPtr{new Lambda(params, body)};
    //     return ExprPtr{new Definition(var, proc)};
    // }
    // normal definition
    auto var = asString(car).value();
    auto value = parse(listBack(cdr));
    return ExprPtr{new Definition(var, value)};
}

inline ExprPtr assignment(MExprPtr const& mexpr)
{
    auto [car, cdr] = deCons(mexpr);
    auto var = asString(car).value();
    auto value = parse(listBack(cdr));
    return ExprPtr{new Assignment(var, value)};
}

inline std::vector<ExprPtr> parseActions(MExprPtr const& mexpr)
{
    std::vector<ExprPtr> actions;
    auto me = mexpr;
    while (me != MNil::instance())
    {
        auto [car, cdr] = deCons(mexpr);
        actions.push_back(parse(car));
        me = cdr;
    }
    return actions;
}

inline std::shared_ptr<Sequence> sequence(MExprPtr const& mexpr)
{
    return std::make_shared<Sequence>(parseActions(mexpr));
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

#if 0
ExprPtr and_()
{
    ASSERT(match({TokenType::kWORD, "and"}));
    return ExprPtr{new And(parseActions())};
}
ExprPtr or_()
{
    ASSERT(match({TokenType::kWORD, "or"}));
    return ExprPtr{new And(parseActions())};
}
ExprPtr application()
{
    auto op = sexpr();
    std::vector<ExprPtr> params = parseActions();
    return ExprPtr{new Application(op, params)};
}
#endif

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
        FAIL("Not implemeneted!");
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
    // else if (carStr == "begin")
    // {
    //     return std::static_pointer_cast<Expr>(sequence(cdr));
    // }
    FAIL("Not implemented!");
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