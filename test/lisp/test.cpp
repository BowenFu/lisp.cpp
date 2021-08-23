#include "lisp/parser.h"
#include "gtest/gtest.h"
#include <cstdlib>

TEST(Lexer, 1)
{
    auto expected = {"(", "define", "square", "(", "lambda", "(", "y", ")", "(", "*", "y", "y", ")", ")", ")", "(", "square", "7", ")"};

    Lexer lex("(define square (lambda (y) (* y y))) (square 7)");
    
    auto t = lex.nextToken();
    for (auto e : expected)
    {
        EXPECT_EQ(t.text, e);
        EXPECT_NE(t.type, TokenType::kEOF);
        t = lex.nextToken();
    }
    EXPECT_EQ(t.type, TokenType::kEOF);
}

TEST(Parser, 1)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition  ( square : Lambda )", "CompoundProcedure"}, {"Application (square)", "49"}};

    Lexer lex("(define square (lambda (y) (* y y))) (square 7)");
    Parser p(lex);
    
    auto env = std::make_shared<Env>();

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
    auto defMul = Definition(ExprPtr{new Variable{"*"}}, ExprPtr{new PrimitiveProcedure{mul}});
    defMul.eval(env);

    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, 2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected =
        {{"Definition  ( factorial : Lambda )", "CompoundProcedure"}, {"Application (factorial)", "120"}};

    Lexer lex("(define factorial (lambda (y) (if (= y 0) 1 (* y (factorial (- y 1)))))) (factorial 5)");
    Parser p(lex);
    
    auto env = std::make_shared<Env>();

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
    auto defMul = Definition(ExprPtr{new Variable{"*"}}, ExprPtr{new PrimitiveProcedure{mul}});
    defMul.eval(env);

    auto gt = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        assert(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        using Bool = Literal<bool>;
        return std::shared_ptr<Expr>(new Bool(num1.get() > num2.get())); 
    };
    auto defGT = Definition(ExprPtr{new Variable{">"}}, ExprPtr{new PrimitiveProcedure{gt}});
    defGT.eval(env);

    auto eq = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        assert(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        using Bool = Literal<bool>;
        return std::shared_ptr<Expr>(new Bool(num1.get() == num2.get())); 
    };
    auto defEQ = Definition(ExprPtr{new Variable{"="}}, ExprPtr{new PrimitiveProcedure{eq}});
    defEQ.eval(env);

    auto sub = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        assert(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        return std::shared_ptr<Expr>(new Number(num1.get() - num2.get())); 
    };
    auto defSub = Definition(ExprPtr{new Variable{"-"}}, ExprPtr{new PrimitiveProcedure{sub}});
    defSub.eval(env);

    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}
