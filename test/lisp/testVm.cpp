#include "gtest/gtest.h"
#include "lisp/vm.h"
#include <numeric>

TEST(VM, add)
{
    std::vector<Byte> const instructions = {kICONST, 0, 0, 0, 1, kICONST, 0, 0, 0, 2, kIADD};
    VM vm{ByteCode{instructions, {}}};
    vm.run();
    auto result = vm.peekOperandStack();
    EXPECT_EQ(std::get<int32_t>(result), 3);
}

TEST(VM, print)
{
    std::vector<Byte> const instructions = {kICONST, 0, 0, 0, 1, kICONST, 0, 0, 0, 2, kIADD, kPRINT};
    VM vm{ByteCode{instructions, {}}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "3\n");
}

TEST(VM, str)
{
    std::vector<Byte> const instructions = {kCONST, 0, 0, 0, 0, kPRINT};
    VM vm{ByteCode{instructions, {"some str: 123"}}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "some str: 123\n");
}

TEST(VM, func)
{
    std::vector<Byte> const instructions = {kICONST, 0, 0, 0, 11, kCALL, 0, 0, 0, 0, kHALT, kLOAD, 0, 0, 0, 0, kPRINT};
    std::vector<Object> const constPool = {FunctionSymbol{"main", 1, 1, 11}};
    VM vm{ByteCode{instructions, constPool}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "11\n");
}

TEST(VM, func2)
{
    std::vector<Byte> const instructions = {kICONST, 0, 0, 0, 11, kCALL, 0, 0, 0, 0, kPRINT, kHALT, kLOAD, 0, 0, 0, 0, kICONST, 0, 0, 0, 2, kIADD, kSTORE, 0, 0, 0, 1, kLOAD, 0, 0, 0, 1, kRET};
    std::vector<Object> const constPool = {FunctionSymbol{"id", 1, 1, 12}};
    VM vm{ByteCode{instructions, constPool}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "13\n");
}