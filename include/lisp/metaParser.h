#ifndef LISP_META_PARSER_H
#define LISP_META_PARSER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/evaluator.h"
#include "lisp/lexer.h"
#include <cctype>

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
        if (c == '#')
        {
            ASSERT(str.size() == 2);
            switch (str[1])
            {
            case 't':
                return true_();
            case 'f':
                return false_();
            
            default:
                FAIL_("Not implemented yet!");
                break;
            }
            return true_();
        }
        if (str == "true")
        {
            return true_();
        }
        if (str == "false")
        {
            return false_();
        }
        if (str == "null")
        {
            return null();
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