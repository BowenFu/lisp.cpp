#include "gtest/gtest.h"
#include "lisp/compiler.h"
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
