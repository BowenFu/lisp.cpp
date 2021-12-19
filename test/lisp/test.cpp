#include "lisp/metaParser.h"
#include "lisp/parser.h"
#include "gtest/gtest.h"
#include <numeric>

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
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition ( square : Lambda )", "CompoundProcedure (y, <procedure-env>)"}, {"(App:square 7)", "49"}};

    Lexer lex("(define square (lambda (y) (* y y))) (square 7)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

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
    auto defMul = Definition("*", ExprPtr{new PrimitiveProcedure{mul}});
    defMul.eval(env);

    for (auto s : expected)
    {
        auto e = parse(p.sexpr());
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}

TEST(Parser, 2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected =
        {{"Definition ( factorial : Lambda )", "CompoundProcedure (y, <procedure-env>)"},
        {"(App:factorial 5)", "120"}};

    Lexer lex("(define factorial (lambda (y) (if (= y 0) 1 (* y (factorial (- y 1)))))) (factorial 5)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

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
    auto defMul = Definition("*", ExprPtr{new PrimitiveProcedure{mul}});
    defMul.eval(env);

    auto gt = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        ASSERT(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        return std::shared_ptr<Expr>(new Bool(num1.get() > num2.get())); 
    };
    auto defGT = Definition(">", ExprPtr{new PrimitiveProcedure{gt}});
    defGT.eval(env);

    auto eq = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        ASSERT(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        return std::shared_ptr<Expr>(new Bool(num1.get() == num2.get())); 
    };
    auto defEQ = Definition("=", ExprPtr{new PrimitiveProcedure{eq}});
    defEQ.eval(env);

    auto sub = [](std::vector<std::shared_ptr<Expr>> const& args)
    {
        ASSERT(args.size() == 2);
        auto num1 = dynamic_cast<Number&>(*args.at(0));
        auto num2 = dynamic_cast<Number&>(*args.at(1));
        return std::shared_ptr<Expr>(new Number(num1.get() - num2.get())); 
    };
    auto defSub = Definition("-", ExprPtr{new PrimitiveProcedure{sub}});
    defSub.eval(env);

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}

TEST(Parser, begin)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition ( square : Lambda )", "CompoundProcedure (y, <procedure-env>)"}, {"(App:square 7)", "49"}};

    Lexer lex("(define square (lambda (y) (* (begin 1 y) y))) (square 7)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

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
    auto defMul = Definition("*", ExprPtr{new PrimitiveProcedure{mul}});
    defMul.eval(env);

    for (auto s : expected)
    {
        auto e = parse(p.sexpr());
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}

TEST(Parser, Assignment)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition ( x : 1 )", "1"}, {"Assignment ( x : 2 )", "2"}};

    Lexer lex("(define x 1) (set! x 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto e = parse(p.sexpr());
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

template <typename C = std::initializer_list<std::string>>
auto strToExpr(C const& strs)
{
    std::vector<ExprPtr> sexprs;
    sexprs.reserve(strs.size());
    std::transform(strs.begin(), strs.end(), std::back_inserter(sexprs),
                   [](auto s)
                   { return ExprPtr{new RawWord{s}}; });
    return sexprs;
}

TEST(vecToCons, 1)
{
    auto strs = {"1", ".", "2"};
    auto sexprs = strToExpr(strs);
    auto cons = vecToCons(sexprs);
    EXPECT_EQ(cons->toString(), "(1 . 2)");
}

TEST(vecToCons, 2)
{
    auto strs = {"1", "2"};
    auto sexprs = strToExpr(strs);
    auto cons = vecToCons(sexprs);
    EXPECT_EQ(cons->toString(), "(1 2)");
}

TEST(vecToCons, exception)
{
    auto strs = {".", "2"};
    auto sexprs = strToExpr(strs);
    EXPECT_THROW(vecToCons(sexprs), std::runtime_error);
}

TEST(MetaParser, Pair)
{
    std::initializer_list<std::string> expected = {"(x . y)"};

    Lexer lex("(x . y)");
    // Lexer lex("(lambda (x . y) (1 2))");
    MetaParser p(lex);
    
    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s);
    }
    EXPECT_TRUE(p.eof());
}

TEST(MetaParser, Pair2)
{
    std::initializer_list<std::string> expected = {"(lambda (x . y) (\"1 () \" 2))"};

    Lexer lex("(lambda (x . y) (\"1 () \" 2))");
    MetaParser p(lex);
    
    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, number)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"-1.2", "-1.2"}};

    Lexer lex("-1.2");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(parse(e)->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, string)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"\" - 1 . 2 () \"", "\" - 1 . 2 () \""}};

    Lexer lex("\" - 1 . 2 () \"");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(parse(e)->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, variable)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"x", "123"}};

    Lexer lex("x");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("x", ExprPtr{new Number{123}});

    for (auto s : expected)
    {
        auto e = p.sexpr();
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(parse(e)->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Definition)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Definition ( x : 1 )", "1"}, {"x", "1"}};

    Lexer lex("(define x 1) x");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Assignment2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Definition ( x : 1 )", "1"},
                                                                   {"Assignment ( x : 2 )", "2"}};

    Lexer lex("(define x 1) (set! x 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Lambda)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Lambda", "CompoundProcedure (<procedure-env>)"}};

    Lexer lex("(lambda () 1)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Lambda2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Lambda", "CompoundProcedure (x, <procedure-env>)"}};

    Lexer lex("(lambda (x) x)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Variadic)
{
    std::initializer_list<std::pair<std::string, std::string> > expected =
        {{"Definition ( to-list : Lambda )", "CompoundProcedure (. y, <procedure-env>)"},
         {"(App:to-list 1)", "(1)"}};

    Lexer lex("(define (to-list . y) y) (to-list 1)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}

TEST(Parser, Variadic2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected =
        {{"Definition ( rest : Lambda )", "CompoundProcedure (_ . y, <procedure-env>)"},
         {"(App:rest 1 2 3)", "(2 3)"}};

    Lexer lex("(define rest (lambda (_ . y) y)) (rest 1 2 3)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}


TEST(Parser, if)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"(if true 1 2)", "1"}};

    Lexer lex("(if #t 1 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Application)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"(App:Lambda)", "1"}};

    Lexer lex("((lambda () 1))");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Application2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Definition ( i : Lambda )", "CompoundProcedure (x, <procedure-env>)"},
                                                                   {"(App:i \".\")", "\".\""}};

    Lexer lex("(define i (lambda (x) x)) (i \".\")");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());

    env->clear();
}

TEST(Parser, begin2)
{
    std::initializer_list<std::pair<std::string, std::string> > expected = {{"Sequence", "2"}};

    Lexer lex("(begin 1 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s.first);
        EXPECT_EQ(e->eval(env)->toString(), s.second);
    }
    EXPECT_TRUE(p.eof());
}
