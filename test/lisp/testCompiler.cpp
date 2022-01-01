#include "gtest/gtest.h"
#include "lisp/compiler.h"
#include "lisp/metaParser.h"
#include "lisp/parser.h"
#include <numeric>

TEST(Compiler, number)
{
    Compiler c{};
    c.compile(ExprPtr{new Number{5.5}});
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5\n");
}

TEST(Compiler, string)
{
    Compiler c{};
    c.compile(ExprPtr{new String{"5.5 abcdefg"}});
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5 abcdefg\n");
}

TEST(Compiler, add)
{
    Compiler c{};
    ExprPtr num1{new Number{5.5}};
    ExprPtr num2{new Number{1.1}};
    ExprPtr num3{new Number{2.2}};
    ExprPtr op{new Variable{"+"}};
    ExprPtr add{new Application{op, {num1, num2, num3}}};
    c.compile(add);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "8.8\n");
}

TEST(Compiler, div)
{
    Compiler c{};
    ExprPtr num1{new Number{5.5}};
    ExprPtr num2{new Number{1.1}};
    ExprPtr op{new Variable{"/"}};
    ExprPtr add{new Application{op, {num1, num2}}};
    c.compile(add);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5\n");
}

TEST(Compiler, bool1)
{
    Compiler c{};
    c.compile(true_());
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "1\n");
}

TEST(Compiler, bool3)
{
    Compiler c{};
    ExprPtr num1{new Number{5.5}};
    ExprPtr num2{new Number{1.1}};
    ExprPtr op1{new Variable{">"}};
    ExprPtr op2{new Variable{"!"}};
    ExprPtr comp{new Application{op1, {num1, num2}}};
    ExprPtr comp2{new Application{op2, {comp}}};
    c.compile(comp2);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "0\n");
}

TEST(Compiler, if)
{
    Compiler c{};
    ExprPtr num1{new Number{5.5}};
    ExprPtr num2{new Number{1.1}};
    ExprPtr op{new Variable{">"}};
    ExprPtr comp{new Application{op, {num1, num2}}};
    ExprPtr max{new If{comp, num1, num2}};
    c.compile(max);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5\n");
}

TEST(Compiler, definition)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    auto const name = "num";
    ExprPtr def{new Definition{name, num}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    c.compile(var);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5\n");
}

TEST(Compiler, lambda0)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    std::shared_ptr<Sequence> seq{new Sequence{{num}}};
    ExprPtr func{new Lambda{Params{std::make_pair(std::vector<std::string>{}, false)}, seq}};
    auto const name = "getNum";
    ExprPtr def{new Definition{name, func}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    c.compile(var);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Closure getNum\n");
}

TEST(Compiler, lambda1)
{
    Compiler c{};
    ExprPtr iVar{new Variable{"i"}};
    std::shared_ptr<Sequence> seq{new Sequence{{iVar}}};
    ExprPtr func{new Lambda{Params{std::make_pair(std::vector<std::string>{"i"}, false)}, seq}};
    auto const name = "identity";
    ExprPtr def{new Definition{name, func}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    c.compile(var);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Closure identity\n");
}

TEST(Compiler, lambda2)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    ExprPtr iVar{new Variable{"i"}};
    std::shared_ptr<Sequence> seq{new Sequence{{iVar}}};
    ExprPtr func{new Lambda{Params{std::make_pair(std::vector<std::string>{"i"}, false)}, seq}};
    auto const name = "identity";
    ExprPtr def{new Definition{name, func}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    ExprPtr app{new Application{var, {num}}};
    c.compile(app);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5\n");
}

TEST(Compiler, lambda3)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    ExprPtr iVar{new Variable{"i"}};
    ExprPtr plusVar{new Variable{"+"}};
    ExprPtr doubleApp{new Application{plusVar, {iVar, iVar}}};
    std::shared_ptr<Sequence> seq{new Sequence{{doubleApp}}};
    ExprPtr func{new Lambda{Params{std::make_pair(std::vector<std::string>{"i"}, false)}, seq}};
    auto const name = "double";
    ExprPtr def{new Definition{name, func}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    ExprPtr app{new Application{var, {num}}};
    c.compile(app);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "11\n");
}

TEST(Compiler, Variadiclambda)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    ExprPtr iVar{new Variable{"i"}};
    std::shared_ptr<Sequence> seq{new Sequence{{iVar}}};
    ExprPtr func{new Lambda{Params{std::make_pair(std::vector<std::string>{"i"}, true)}, seq}};
    auto const name = "list";
    ExprPtr def{new Definition{name, func}};
    c.compile(def);
    ExprPtr var{new Variable{name}};
    ExprPtr app{new Application{var, {num}}};
    c.compile(app);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "(5.5 . nil)\n");
}

TEST(Compiler, consCar)
{
    Compiler c{};
    ExprPtr num{new Number{5.5}};
    auto const name1 = "cons";
    ExprPtr var1{new Variable{name1}};
    ExprPtr app1{new Application{var1, {num, num}}};
    auto const name2 = "cdr";
    ExprPtr var2{new Variable{name2}};
    ExprPtr app2{new Application{var2, {app1}}};
    c.compile(app2);
    ByteCode code = c.code();
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "5.5\n");
}

auto sourceToBytecode(std::string const& source)
{
    Lexer lex(source);
    MetaParser p(lex);
    
    Compiler c{};
    while (!p.eof())
    {
        auto e = parse(p.sexpr());
        c.compile(e);
    }

    return c.code();
}

TEST(Compiler, square)
{
    std::string const source = "(define square (lambda (y) (* y y))) (square 7)";
    auto code = sourceToBytecode(source);
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "49\n");
}

TEST(Compiler, factorial)
{
    std::string const source = "(define factorial (lambda (y) (if (= y 0) 1 (* y (factorial (- y 1)))))) (factorial 5)";
    auto code = sourceToBytecode(source);
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "120\n");
}

TEST(Compiler, rest)
{
    std::string const source = "(define rest (lambda (_ . y) y)) (rest 1 2 3)";
    auto code = sourceToBytecode(source);
    code.instructions.push_back(kPRINT);
    VM vm{code};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "(2 . (3 . nil))\n");
}

// TEST(Compiler, localBinding)
// {
//     std::string const source = " (define (my-cons car cdr) (lambda (dispatch) (if (= dispatch 'my-car) car cdr))) ((my-cons 1 2) 'my-cdr)";
//     auto code = sourceToBytecode(source);
//     code.instructions.push_back(kPRINT);
//     VM vm{code};
//     testing::internal::CaptureStdout();
//     vm.run();
//     std::string output = testing::internal::GetCapturedStdout();
//     EXPECT_EQ(output, "2\n");
// }
