#ifndef LISP_LEXER_H
#define LISP_LEXER_H

#include <string>
#include <sstream>
#include <iostream>
#include "lisp/evaluator.h"
#include <cctype>

enum class TokenType
{
    kL_PAREN,
    kR_PAREN,
    kQUOTE,
    kQUASI_QUOTE,
    kUNQUOTE,
    kWORD,
    kEOF
};

struct Token
{
    TokenType type;
    std::string text;
};

inline bool operator==(Token const& lhs, Token const& rhs)
{
    return lhs.type == rhs.type && lhs.text == rhs.text;
}

template <typename T, typename C = std::initializer_list<T>>
bool elem(T t, C c)
{
    return std::any_of(c.begin(), c.end(), [t](T e){ return e == t; });
}

class Lexer
{
public:
    Lexer(std::string const& input)
    : mInput{input}
    , mPos{}
    {}
    bool isWS(char c)
    {
        return elem(c, {' ', '\t', '\n', '\r'});
    }
    void consume()
    {
        ++mPos;
    }
    Token nextToken()
    {
        while (mPos < mInput.size())
        {
            auto c = mInput.at(mPos);
            if (c == '"')
            {
                return stringToken();
            }
            if (isWS(c))
            {
                consume();
                continue;
            }
            switch(c)
            {
            case '(':
                consume();
                return Token{TokenType::kL_PAREN, std::string{c}};
            case ')':
                consume();
                return Token{TokenType::kR_PAREN, std::string{c}};
            case '\'':
                consume();
                return Token{TokenType::kQUOTE, std::string{c}};
            case '`':
                consume();
                return Token{TokenType::kQUASI_QUOTE, std::string{c}};
            case ',':
                consume();
                return Token{TokenType::kUNQUOTE, std::string{c}};
            default:
                return wordToken();
            }
        }
        return Token{TokenType::kEOF, "<EOF>"};
    }
    Token wordToken()
    {
        std::ostringstream word{};
        while (mPos < mInput.size())
        {
            auto c = mInput.at(mPos);
            if(isWS(c) || elem(c, {'(', ')'}))
            {
                break;
            }
            word << c;
            consume();
        }
        std::string wordStr = word.str();
        if(wordStr.empty())
        {
            throw std::runtime_error{"empty word token"};
        }
        return Token{TokenType::kWORD, wordStr};
    }
    Token stringToken()
    {
        ASSERT(mInput.at(mPos) == '"');
        size_t begin = mPos;
        auto const endIter = std::find(mInput.begin() + static_cast<long>(mPos) + 1U, mInput.end(), '"');
        ASSERT(endIter != mInput.end());
        size_t end = static_cast<size_t>(endIter - mInput.begin()) + 1U;
        std::string wordStr = mInput.substr(begin, end - begin);
        if(wordStr.empty())
        {
            throw std::runtime_error{"empty word token"};
        }
        mPos = end;
        return Token{TokenType::kWORD, wordStr};
    }
private:
    std::string mInput;
    size_t mPos;
};

#endif // LISP_LEXER_H