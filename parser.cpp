#include <string>
#include <sstream>
#include <iostream>

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
    void match(TokenType t)
    {
        if (mLookAhead.type == t)
        {
            consume();
        }
        else
        {
            throw std::runtime_error{"TokenType Mismatch"};
        }
    }
    void match(Token const& token)
    {
        if (mLookAhead == token)
        {
            consume();
        }
        else
        {
            throw std::runtime_error{"Token Mismatch"};
        }
    }
    void atomic()
    {
        match(TokenType::kWORD);
    }
    void list()
    {
        match(TokenType::kL_PAREN);
        listContext();
        match(TokenType::kR_PAREN);
    }
    void sexpr()
    {
        if (mLookAhead.type == TokenType::kL_PAREN)
        {
            return list();
        }
        return atomic();
    }
    void listContext()
    {
        if (mLookAhead.type == TokenType::kWORD)
        {
            if (mLookAhead.text == "define")
            {
                definition();
                return;
            }
            else if (mLookAhead.text == "set!")
            {
                assignment();
                return;
            }
            else
            {
                application();
                return;
            }
        }
        else
        {
            assert(!"Not implemented");
        }
    }
    void definition()
    {
        match({TokenType::kWORD, "define"});
        match(TokenType::kWORD);
        sexpr();
    }
    void assignment()
    {
        match({TokenType::kWORD, "set!"});
        match(TokenType::kWORD);
        sexpr();
    }
    void application()
    {
        match(TokenType::kWORD); // op
        // params...
        sexpr();
    }
private:
    Lexer mInput;
    Token mLookAhead;
};

int32_t main()
{
    Lexer lex("(define x 1)");
    Parser p(lex);
    
    auto t = lex.nextToken();
    while (t.type != TokenType::kEOF)
    {
        std::cout << t.text << std::endl;
        t = lex.nextToken();
    }

    p.sexpr();

    return 0;
}