#include "lisp/vm.h"
#include "lisp/meta.h"
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
    while (mIp < mCode.instructions.size())
    {
        Byte opCode = mCode.instructions[mIp];
        ++mIp;
        switch (opCode)
        {
        case kICONST:
        {
            int32_t word = fourBytesToInteger<int32_t>(&mCode.instructions[mIp]);
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
        case kADD:
        case kSUB:
        case kMUL:
        case kDIV:
        {
            auto const rhs = operandStack().top();
            operandStack().pop();
            auto const lhs = operandStack().top();
            operandStack().pop();
            if (auto lhsDPtr = std::get_if<double>(&lhs))
            {
                auto lhsD = *lhsDPtr;
                auto rhsD = std::get<double>(rhs);
                double result{};
                switch (opCode)
                {
                case kADD:
                    result = lhsD + rhsD;
                    break;
                
                case kSUB:
                    result = lhsD - rhsD;
                    break;
                
                case kMUL:
                    result = lhsD * rhsD;
                    break;
                
                case kDIV:
                    result = lhsD / rhsD;
                    break;
                
                default:
                    break;
                }
                operandStack().push(result);
            }
            else
            {
                FAIL_("Not supported yet!");
            }
            break;
        }
        case kCONST:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;
            operandStack().push(mCode.constantPool.at(index));
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
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;

            auto const& functionSymbol = std::get<FunctionSymbol>(mCode.constantPool.at(index));
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
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;

            operandStack().push(mCallStack.top().locals(index));
            break;
        }
        case kSTORE:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;

            mCallStack.top().locals(index) = operandStack().top();
            operandStack().pop();
            break;
        }
        }
    }
}