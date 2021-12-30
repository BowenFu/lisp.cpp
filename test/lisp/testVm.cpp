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
