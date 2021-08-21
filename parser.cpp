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
        return elem(c, {'\t', '\n', '\r'});
    }
    Token nextToken()
    {
        while (mPos < mInput.size())
        {
            auto c = mInput.at(mPos);
            if (isWS(c))
            {
                continue;
            }
            switch(c)
            {
            case '(':
                ++mPos;
                return Token{TokenType::kL_PAREN, std::string{c}};
            case ')':
                ++mPos;
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
            ++mPos;
        }
        std::string wordStr = word.str();
        assert(!wordStr.empty());
        return Token{TokenType::kWORD, wordStr};
    }
private:
    std::string mInput;
    int32_t mPos;
};

int32_t main()
{
    Lexer lex("(x)");
    auto t = lex.nextToken();
    while (t.type != TokenType::kEOF)
    {
        std::cout << t.text << std::endl;
        t = lex.nextToken();
    }

    return 0;
}