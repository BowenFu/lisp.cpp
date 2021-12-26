#include "lisp/vm.h"
#include <iostream>

std::ostream& operator << (std::ostream& o, StackFrame const& f)
{
    return o << "StackFrame " << f.func().name();
}

std::ostream& operator << (std::ostream& o, FunctionSymbol const& f)
{
    return o << "Function " << f.name();
}

void VM::run()
{
    while (mIp < mCode.size())
    {
        Byte bytecode = mCode[mIp];
        ++mIp;
        switch (bytecode)
        {
        case kICONST:
        {
            int32_t word = fourBytesToInteger<int32_t>(&mCode[mIp]);
            mIp += 4;
            operandStack().push(word);
            break;
        }
        case kIADD:
        {
            int32_t rhs = std::get<int32_t>(operandStack().top());
            operandStack().pop();
            int32_t lhs = std::get<int32_t>(operandStack().top());
            operandStack().pop();
            int32_t result = lhs + rhs;
            operandStack().push(result);
            break;
        }
        case kSCONST:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode[mIp]);
            mIp += 4;
            operandStack().push(mConstantPool.at(index));
            break;
        }
        case kPRINT:
        {
            auto op = operandStack().top();
            operandStack().pop();
            std::visit([](auto op)
            {
                std::cout << op << std::endl;
            }, op);
            break;
        }
        case kHALT:
            return;
        case kCALL:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode[mIp]);
            mIp += 4;

            auto const& functionSymbol = std::get<FunctionSymbol>(mConstantPool.at(index));
            std::vector<Object> params(functionSymbol.nbArgs() + functionSymbol.nbLocals());
            for (size_t i = functionSymbol.nbArgs(); i > 0; --i)
            {
                params.at(i - 1) = operandStack().top();
                operandStack().pop();
            }
            mCallStack.push(StackFrame{functionSymbol, std::move(params), mIp});
            mIp = functionSymbol.address();
            break;
        }
        case kRET:
        {
            mIp = mCallStack.top().returnAddress();
            mCallStack.pop();
            break;
        }
        case kLOAD:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode[mIp]);
            mIp += 4;

            operandStack().push(mCallStack.top().locals(index));
            break;
        }
        case kSTORE:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode[mIp]);
            mIp += 4;

            mCallStack.top().locals(index) = operandStack().top();
            operandStack().pop();
            break;
        }
        }
    }
}