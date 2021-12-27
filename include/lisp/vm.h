#ifndef LISP_VM_H
#define LISP_VM_H

#include <cstdint>
#include <vector>
#include <stack>
#include <string>
#include <variant>

using Byte = uint8_t;

enum OpCode : Byte
{
    kICONST,
    kIADD,
    kADD,
    kSUB,
    kMUL,
    kDIV,
    kCONST,
    kHALT,
    kPRINT,
    kCALL,
    kRET,
    kLOAD,
    kSTORE,
    kTRUE,
    kFALSE,
    kEQUAL,
    kNOT_EQUAL,
    kGREATER_THAN,
    kNOT,
    kMINUS,
    kJUMP,
    kJUMP_IF_NOT_TRUE,
};


class FunctionSymbol
{
    std::string mName{};
    size_t mNbArgs{};
    size_t mNbLocals{};
    size_t mAddress{};
public:
    FunctionSymbol(std::string name, size_t nbArgs, size_t nbLocals, size_t address)
    : mName{name}
    , mNbArgs{nbArgs}
    , mNbLocals{nbLocals}
    , mAddress{address}
    {}
    std::string name() const
    {
        return mName;
    }
    size_t nbArgs() const
    {
        return mNbArgs;
    }
    size_t nbLocals() const
    {
        return mNbLocals;
    }
    size_t address() const
    {
        return mAddress;
    }
};

using Object = std::variant<int32_t, double, std::string, FunctionSymbol>;

class VM;

class StackFrame
{
    FunctionSymbol const& mFunc;
    std::vector<Object> mLocals;
    size_t mReturnAddress;
public:
    StackFrame(FunctionSymbol const& func, std::vector<Object>&& locals, size_t returnAddress)
    : mFunc{func}
    , mLocals{std::move(locals)}
    , mReturnAddress{returnAddress}
    {
    }
    auto func() const
    {
        return mFunc;
    }
    auto returnAddress() const
    {
        return mReturnAddress;
    }
    auto& locals(size_t i)
    {
        return mLocals.at(i);
    }
};

using Instructions = std::vector<Byte>;

class ByteCode
{
public:
    Instructions instructions{};
    std::vector<Object> constantPool{};
};

class VM
{
public:
    VM(ByteCode const& code)
    : mCode{code}
    {}
    void run();
    auto peekOperandStack() const
    {
        return mOperands.top();
    }
    auto& operandStack()
    {
        return mOperands;
    }
private:
    ByteCode mCode{};
    size_t mIp{};
    std::stack<Object> mOperands{};
    std::stack<StackFrame> mCallStack{};
};

template <typename T>
auto fourBytesToInteger(Byte const* buffer) -> T
{
    return static_cast<T>(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]);
}

#endif // LISP_VM_H