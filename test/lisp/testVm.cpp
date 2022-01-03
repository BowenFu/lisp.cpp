#include "gtest/gtest.h"
#include "lisp/vm.h"
#include <numeric>

TEST(VM, add)
{
    std::vector<vm::Byte> const instructions = {vm::kICONST, 0, 0, 0, 1, vm::kICONST, 0, 0, 0, 2, vm::kIADD};
    vm::VM vm{vm::ByteCode{instructions, {}}};
    vm.run();
    auto result = vm.peekOperandStack();
    EXPECT_EQ(std::get<vm::Int>(result).value, 3);
}

TEST(VM, print)
{
    std::vector<vm::Byte> const instructions = {vm::kICONST, 0, 0, 0, 1, vm::kICONST, 0, 0, 0, 2, vm::kIADD, vm::kPRINT};
    vm::VM vm{vm::ByteCode{instructions, {}}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "3\n");
}

TEST(VM, str)
{
    std::vector<vm::Byte> const instructions = {vm::kCONST, 0, 0, 0, 0, vm::kPRINT};
    vm::VM vm{vm::ByteCode{instructions, {vm::String{"some str: 123"}}}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "\"some str: 123\"\n");
}

TEST(VM, cons)
{
    std::vector<vm::Byte> const instructions = {vm::kCONST, 0, 0, 0, 0, vm::kCONST, 0, 0, 0, 1, vm::kCONS, vm::kCAR, vm::kPRINT};
    vm::VM vm{vm::ByteCode{instructions, {vm::String{"some str: 123"}, vm::Int{12345}}}};
    testing::internal::CaptureStdout();
    vm.run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "\"some str: 123\"\n");
}
