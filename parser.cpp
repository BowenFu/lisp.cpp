#include <string>
#include <sstream>
#include <iostream>
#include "evaluator.h"
#include <cctype>

enum class TokenType
{
    kL_PAREN,
    kR_PAREN,
    kWORD,
    kEOF
};

struct Token
{
    TokenType type;
    std::string text;
};

bool operator==(Token const& lhs, Token const& rhs)
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
        assert(!wordStr.empty());
        return Token{TokenType::kWORD, wordStr};
    }
private:
    std::string mInput;
    int32_t mPos;
};

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
    ExprPtr number()
    {
        double num = std::stod(mLookAhead.text);
        auto result = ExprPtr{new Literal<double>(num)};
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
        assert(mLookAhead.type == TokenType::kWORD);
        if (isdigit(mLookAhead.text.front()))
        {
            return number();
        }
        return variable();
    }
    ExprPtr list()
    {
        assert(match(TokenType::kL_PAREN));
        auto result = listContext();
        assert(result);
        assert(match(TokenType::kR_PAREN));
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
            else
            {
                return application();
            }
        }
        else
        {
            assert(!"Not implemented");
        }
        return {};
    }
    ExprPtr definition()
    {
        assert(match({TokenType::kWORD, "define"}));
        auto var = variable();
        auto value = sexpr();
        return ExprPtr{new Definition(var, value)};
    }
    ExprPtr assignment()
    {
        assert(match({TokenType::kWORD, "set!"}));
        auto var = variable();
        auto value = sexpr();
        return ExprPtr{new Definition(var, value)};
    }
    ExprPtr application()
    {
        match(TokenType::kWORD); // op
        // params...
        sexpr();
        return {};
    }
private:
    Lexer mInput;
    Token mLookAhead;
};

int32_t main()
{
    Lexer lex("(define x 1) x");
    Parser p(lex);
    
    auto t = lex.nextToken();
    while (t.type != TokenType::kEOF)
    {
        std::cout << t.text << std::endl;
        t = lex.nextToken();
    }

    Env env{};
    auto e = p.sexpr();
    std::cout << e->toString() << std::endl;
    std::cout << e->eval(env)->toString() << std::endl;

    e = p.sexpr();
    std::cout << e->toString() << std::endl;
    std::cout << e->eval(env)->toString() << std::endl;

    return 0;
}