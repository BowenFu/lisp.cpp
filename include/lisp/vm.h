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
    kGET_LOCAL,
    kSET_LOCAL,
    kTRUE,
    kFALSE,
    kEQUAL,
    kNOT_EQUAL,
    kGREATER_THAN,
    kNOT,
    kMINUS,
    kJUMP,
    kJUMP_IF_NOT_TRUE,
    kSET_GLOBAL,
    kGET_GLOBAL,
    kPOP,
};

using Instructions = std::vector<Byte>;

class FunctionSymbol
{
    std::string mName{};
    size_t mNbArgs{};
    size_t mNbLocals{};
    Instructions mInstructions{};
public:
    FunctionSymbol(size_t nbArgs, size_t nbLocals, Instructions const& instructions)
    : mName{}
    , mNbArgs{nbArgs}
    , mNbLocals{nbLocals}
    , mInstructions{instructions}
    {}
    void setName(std::string const& name)
    {
        mName = name;
    }
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
    auto const& instructions() const
    {
        return mInstructions;
    }
};

class VMCons;
using ConsPtr = std::shared_ptr<VMCons>;

using Object = std::variant<int32_t, double, std::string, FunctionSymbol, ConsPtr>;

class VMCons
{
    Object mCar{};
    Object mCdr{};
public:
    VMCons(Object const& car_, Object const& cdr_)
    : mCar{car_}
    , mCdr{cdr_}
    {}
    auto const& car() const
    {
        return mCar;
    }
    auto const& cdr() const
    {
        return mCdr;
    }
};

inline ConsPtr cons(Object const& car_, Object const& cdr_)
{
    return std::make_shared<VMCons>(car_, cdr_);
}

inline Object car(ConsPtr const& cons_)
{
    return cons_->car();
}

inline Object cdr(ConsPtr const& cons_)
{
    return cons_->cdr();
}

class VM;

class StackFrame
{
    FunctionSymbol const mFunc;
    std::vector<Object> mLocals;
    size_t mReturnAddress;
public:
    StackFrame(FunctionSymbol const& func, std::vector<Object>&& locals, size_t returnAddress)
    : mFunc{func}
    , mLocals{std::move(locals)}
    , mReturnAddress{returnAddress}
    {
    }
    auto const& func() const
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
    auto const& instructions() const
    {
        return mCallStack.empty() ? mCode.instructions : mCallStack.top().func().instructions();
    }
private:
    ByteCode mCode{};
    size_t mIp{};
    std::vector<Object> mGlobals{};
    std::stack<Object> mOperands{};
    std::stack<StackFrame> mCallStack{};
};

template <typename T>
auto fourBytesToInteger(Byte const* buffer) -> T
{
    return static_cast<T>(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]);
}

#endif // LISP_VM_H