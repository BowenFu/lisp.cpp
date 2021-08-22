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
    bool eof() const
    {
        return mLookAhead.type == TokenType::kEOF;
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
            else if (mLookAhead.text == "lambda")
            {
                return lambda();
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
    auto sequence()
    {
        std::vector<ExprPtr> actions;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            actions.push_back(sexpr());
        }
        return std::make_shared<Sequence>(actions);
    }
    ExprPtr lambda()
    {
        assert(match({TokenType::kWORD, "lambda"}));
        assert(match(TokenType::kL_PAREN));
        std::vector<std::string> params;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            params.push_back(dynamic_cast<Variable*>(variable().get())->name());
        }
        assert(match(TokenType::kR_PAREN));
        auto body = sequence();
        return ExprPtr{new Lambda(params, body)};
    }
    ExprPtr application()
    {
        auto op = sexpr();
        std::vector<ExprPtr> params;
        while (mLookAhead.type != TokenType::kR_PAREN)
        {
            params.push_back(sexpr());
        }
        
        return ExprPtr{new Application(op, params)};
    }
private:
    Lexer mInput;
    Token mLookAhead;
};

int32_t main()
{
    Lexer lex("(define square (lambda (y) (* y y))) (square 7)");
    Parser p(lex);
    
    auto t = lex.nextToken();
    while (t.type != TokenType::kEOF)
    {
        std::cout << t.text << std::endl;
        t = lex.nextToken();
    }

    Env env{};

    using Number = Literal<double>;
    auto mul = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        auto result = std::accumulate(args.begin(), args.end(), 1.0, [](auto p, std::shared_ptr<Expr> const& arg)
        {
            auto num = dynamic_cast<Number&>(*arg);
            return p * num.get();
        }
        );
        return std::shared_ptr<Expr>(new Number(result)); 
    };
    auto mulOp = ExprPtr{new PrimitiveProcedure{mul}};
    auto variableMul = ExprPtr{new Variable{"*"}};
    auto defMul = Definition(variableMul, mulOp);
    defMul.eval(env);

    auto e = p.sexpr();
    while (true)
    {
        std::cout << e->toString() << std::endl;
        std::cout << e->eval(env)->toString() << std::endl;
        if (p.eof())
        {
            break;
        }
        e = p.sexpr();
    }
    
    return 0;
}