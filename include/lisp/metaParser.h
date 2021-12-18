#ifndef LISP_META_PARSER_H
#define LISP_META_PARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/evaluator.h"
#include "lisp/lexer.h"
#include <cctype>

inline auto vecToCons(std::vector<ExprPtr> const& vec)
{
    auto result = nil();
    auto vecSize = vec.size();
    auto i = vec.rbegin();
    if (vecSize >= 2)
    {
        auto dot = vec.at(vecSize - 2);
        auto dotPtr = dynamic_cast<RawWord*>(dot.get());
        if (dotPtr != nullptr && dotPtr->toString() == ".")
        {
            ASSERT(vecSize >=3);
            ++i;
            ++i;
            result = vec.back();
        }
    }
    for (;i != vec.rend(); ++i)
    {
        result = ExprPtr{new Cons{*i, result}};
    }
    return result;
}

class MetaParser
{
public:
    MetaParser(Lexer const& input)
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

    auto parseAtomic(std::string const& str) -> ExprPtr
    {
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
        return ExprPtr{new RawWord(str)};
    }

    ExprPtr atomic()
    {
        if (mLookAhead.type != TokenType::kWORD)
        {
            throw std::runtime_error(mLookAhead.text);
        }
        auto result = parseAtomic(mLookAhead.text);
        consume();
        return result;
    }
    ExprPtr parenthesized()
    {
        ASSERT(match(TokenType::kL_PAREN));
        auto result =  cons();
        ASSERT(result);
        ASSERT(match(TokenType::kR_PAREN));
        return result;
    }
    ExprPtr cons()
    {
        std::vector<ExprPtr> actions;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            actions.push_back(sexpr());
        }
        return vecToCons(actions);
    }
    ExprPtr sexpr()
    {
        switch (mLookAhead.type)
        {
        case TokenType::kQUOTE:
            consume();
            return vecToCons({ExprPtr{new RawWord{"quote"}}, sexpr()});
        case TokenType::kQUASI_QUOTE:
            consume();
            return vecToCons({ExprPtr{new RawWord{"quasiquote"}}, sexpr()});
        case TokenType::kUNQUOTE:
            consume();
            return vecToCons({ExprPtr{new RawWord{"unquote"}}, sexpr()});
        case TokenType::kUNQUOTE_SPLICING:
            consume();
            return vecToCons({ExprPtr{new RawWord{"unquote-splicing"}}, sexpr()});
        case TokenType::kL_PAREN:
            return parenthesized();
        default:
            break;
        }
        return atomic();
    }
private:
    Lexer mInput;
    Token mLookAhead;
};


#endif // LISP_META_PARSER_H