#ifndef LISP_META_PARSER_H
#define LISP_META_PARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/meta.h"
#include "lisp/lexer.h"
#include <cctype>

inline auto vecToMCons(std::vector<MExprPtr> const& vec)
{
    auto result = MNil::instance();
    auto vecSize = vec.size();
    auto i = vec.rbegin();
    if (vecSize >= 2)
    {
        auto dot = vec.at(vecSize - 2);
        auto dotPtr = dynamic_cast<MAtomic*>(dot.get());
        if (dotPtr != nullptr && dotPtr->get() == ".")
        {
            ASSERT(vecSize >=3);
            ++i;
            ++i;
            result = vec.back();
        }
    }
    for (;i != vec.rend(); ++i)
    {
        result = MExprPtr{new MCons{*i, result}};
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

    MExprPtr atomic()
    {
        if (mLookAhead.type != TokenType::kWORD)
        {
            throw std::runtime_error(mLookAhead.text);
        }
        auto result = MExprPtr{new MAtomic(mLookAhead.text)};
        consume();
        return result;
    }
    MExprPtr parenthesized()
    {
        ASSERT(match(TokenType::kL_PAREN));
        auto result =  cons();
        ASSERT(result);
        ASSERT(match(TokenType::kR_PAREN));
        return result;
    }
    MExprPtr cons()
    {
        std::vector<MExprPtr> actions;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            actions.push_back(sexpr());
        }
        return vecToMCons(actions);
    }
    MExprPtr sexpr()
    {
        if (mLookAhead.type == TokenType::kQUOTE)
        {
            consume();
            return vecToMCons({MExprPtr{new MAtomic{"quote"}}, sexpr()});
        }
        if (mLookAhead.type == TokenType::kL_PAREN)
        {
            return parenthesized();
        }
        return atomic();
    }
private:
    Lexer mInput;
    Token mLookAhead;
};

#endif // LISP_META_PARSER_H