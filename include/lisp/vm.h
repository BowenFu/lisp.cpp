#ifndef LISP_VM_H
#define LISP_VM_H

#include <cstdint>
#include <vector>
#include <stack>
#include <string>
#include <variant>
#include <memory>

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
    kSET_GLOBAL,
    kGET_GLOBAL,
    kGET_FREE,
    kTRUE,
    kFALSE,
    kEQUAL,
    kNOT_EQUAL,
    kGREATER_THAN,
    kNOT,
    kMINUS,
    kJUMP,
    kJUMP_IF_NOT_TRUE,
    kPOP,
    kCONS,
    kCAR,
    kCDR,
    kCURRENT_FUNCTION,
    kCLOSURE
};

using Instructions = std::vector<Byte>;

class FunctionSymbol
{
    std::string mName{};
    size_t mNbArgs{};
    bool mVariadic{};
    size_t mNbLocals{};
    Instructions mInstructions{};
public:
    FunctionSymbol(std::string const& name, size_t nbArgs, bool variadic, size_t nbLocals, Instructions const& instructions)
    : mName{name}
    , mNbArgs{nbArgs}
    , mVariadic{variadic}
    , mNbLocals{nbLocals}
    , mInstructions{instructions}
    {}
    std::string name() const
    {
        return mName;
    }
    size_t nbArgs() const
    {
        return mNbArgs;
    }
    auto variadic() const
    {
        return mVariadic;
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

class Closure;
using ClosurePtr = std::shared_ptr<Closure>;

class VMCons;
using ConsPtr = std::shared_ptr<VMCons>;


class VMNil
{};

inline constexpr VMNil vmNil{};

using Object = std::variant<int32_t, double, std::string, FunctionSymbol, ClosurePtr, ConsPtr, VMNil>;

class Closure
{
    FunctionSymbol mFuncSym;
    std::vector<Object> mFreeVars;
public:
    Closure(FunctionSymbol const& funcSym, std::vector<Object> const& freeVars)
    : mFuncSym{funcSym}
    , mFreeVars{freeVars}
    {
    }
    auto const& funcSym() const
    {
        return mFuncSym;
    }
    auto const& freeVars() const
    {
        return mFreeVars;
    }
};

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
    ClosurePtr const mClosure;
    std::vector<Object> mLocals;
    size_t mReturnAddress;
public:
    StackFrame(ClosurePtr const& func, std::vector<Object>&& locals, size_t returnAddress)
    : mClosure{func}
    , mLocals{std::move(locals)}
    , mReturnAddress{returnAddress}
    {
    }
    auto const& closure() const
    {
        return mClosure;
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
        return mCallStack.empty() ? mCode.instructions : mCallStack.top().closure()->funcSym().instructions();
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