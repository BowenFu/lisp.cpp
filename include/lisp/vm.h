#include <cstdint>
#include <vector>
#include <stack>
#include <string>
#include <variant>

using Byte = uint8_t;

enum Instruction
{
    kICONST,
    kIADD,
    kSCONST,
    kHALT,
    kPRINT,
    kCALL,
    kRET,
    kLOAD,
    kSTORE,
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

using Object = std::variant<int32_t, float, std::string, FunctionSymbol>;

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

class VM
{
public:
    VM(std::vector<Byte> const& code, std::vector<Object> const& constantPool = {})
    : mCode{code}
    , mConstantPool{constantPool}
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
    std::vector<Byte> mCode{};
    std::vector<Object> mConstantPool{};
    size_t mIp{};
    std::stack<Object> mOperands{};
    std::stack<StackFrame> mCallStack{};
};

template <typename T>
auto fourBytesToInteger(Byte const* buffer) -> T
{
    return static_cast<T>(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]);
}