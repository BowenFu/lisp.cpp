#include "lisp/metaParser.h"
#include "lisp/parser.h"
#include "gtest/gtest.h"

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
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition ( square : Lambda )", "CompoundProcedure (y, <procedure-env>)"}, {"Application (square)", "49"}};

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
}

TEST(Parser, 2)
{
    std::initializer_list<std::array<std::string, 3> > expected =
        {{"(define factorial (lambda (y) (if (= y 0) 1 (* y (factorial (- y 1))))))", "Definition ( factorial : Lambda )", "CompoundProcedure (y, <procedure-env>)"},
        {"(factorial 5)", "Application (factorial)", "120"}};

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
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, begin)
{
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"Definition ( square : Lambda )", "CompoundProcedure (y, <procedure-env>)"}, {"Application (square)", "49"}};

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
auto strToMExpr(C const& strs)
{
    std::vector<MExprPtr> sexprs;
    sexprs.reserve(strs.size());
    std::transform(strs.begin(), strs.end(), std::back_inserter(sexprs),
                   [](auto s)
                   { return MExprPtr{new MAtomic{s}}; });
    return sexprs;
}

TEST(vecToMCons, 1)
{
    auto strs = {"1", ".", "2"};
    auto sexprs = strToMExpr(strs);
    auto cons = vecToMCons(sexprs);
    EXPECT_EQ(cons->toString(), "(1 . 2)");
}

TEST(vecToMCons, 2)
{
    auto strs = {"1", "2"};
    auto sexprs = strToMExpr(strs);
    auto cons = vecToMCons(sexprs);
    EXPECT_EQ(cons->toString(), "(1 2)");
}

TEST(vecToMCons, exception)
{
    auto strs = {".", "2"};
    auto sexprs = strToMExpr(strs);
    EXPECT_THROW(vecToMCons(sexprs), std::runtime_error);
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
    std::initializer_list<std::pair<std::string, std::string>> expected = {{"\" - 1 . 2 () \"", " - 1 . 2 () "}};

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
    std::initializer_list<std::array<std::string, 3> > expected = {{"(define x 1)", "Definition ( x : 1 )", "1"}, {"x", "x", "1"}};

    Lexer lex("(define x 1) x");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Assignment2)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(define x 1)", "Definition ( x : 1 )", "1"},
                                                                   {"(set! x 2)", "Assignment ( x : 2 )", "2"}};

    Lexer lex("(define x 1) (set! x 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Lambda)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(lambda () 1)", "Lambda", "CompoundProcedure (<procedure-env>)"}};

    Lexer lex("(lambda () 1)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Lambda2)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(lambda (x) x)", "Lambda", "CompoundProcedure (x, <procedure-env>)"}};

    Lexer lex("(lambda (x) x)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, if)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(if #t 1 2)", "(if #t 1 2)", "1"}};

    Lexer lex("(if #t 1 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("#t", true_());

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, cond)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(cond (#t 1) (else 2))", "Cond", "1"}};

    Lexer lex("(cond (#t 1) (else 2))");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("#t", true_());

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Application)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"((lambda () 1))", "Application (Lambda)", "1"}};

    Lexer lex("((lambda () 1))");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("#t", true_());

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, Application2)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(define i (lambda (x) x))", "Definition ( i : Lambda )", "CompoundProcedure (x, <procedure-env>)"},
                                                                   {"(i \".\")", "Application (i)", "."}};

    Lexer lex("(define i (lambda (x) x)) (i \".\")");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("#t", true_());

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, begin2)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(begin 1 2)", "Sequence", "2"}};

    Lexer lex("(begin 1 2)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}

TEST(Parser, andor)
{
    std::initializer_list<std::array<std::string, 3> > expected = {{"(or 1 2 (and 3) #t)", "Or", "true"}};

    Lexer lex("(or 1 2 (and 3) #t)");
    MetaParser p(lex);
    
    auto env = std::make_shared<Env>();
    env->defineVariable("#t", true_());

    for (auto s : expected)
    {
        auto me = p.sexpr();
        EXPECT_EQ(me->toString(), s[0]);
        auto e = parse(me);
        EXPECT_EQ(e->toString(), s[1]);
        EXPECT_EQ(e->eval(env)->toString(), s[2]);
    }
    EXPECT_TRUE(p.eof());
}