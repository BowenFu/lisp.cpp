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
        case kEQUAL:
        case kNOT_EQUAL:
        case kGREATER_THAN:
        {
            auto const rhs = operandStack().top();
            operandStack().pop();
            auto const lhs = operandStack().top();
            operandStack().pop();
            if (auto lhsDPtr = std::get_if<double>(&lhs))
            {
                auto lhsD = *lhsDPtr;
                auto rhsD = std::get<double>(rhs);
                switch (opCode)
                {
                case kADD:
                {
                    double result = lhsD + rhsD;
                    operandStack().push(result);
                    break;
                }
                
                case kSUB:
                {
                    double result = lhsD - rhsD;
                    operandStack().push(result);
                    break;
                }
                
                case kMUL:
                {
                    double result = lhsD * rhsD;
                    operandStack().push(result);
                    break;
                }
                
                case kDIV:
                {
                    double result = lhsD / rhsD;
                    operandStack().push(result);
                    break;
                }
                
                case kEQUAL:
                {
                    bool result = lhsD == rhsD;
                    operandStack().push(result);
                    break;
                }

                case kNOT_EQUAL:
                {
                    bool result = lhsD != rhsD;
                    operandStack().push(result);
                    break;
                }

                case kGREATER_THAN:
                {
                    bool result = lhsD > rhsD;
                    operandStack().push(result);
                    break;
                }

                default:
                    break;
                }
            }
            else
            {
                FAIL_("Not supported yet!");
            }
            break;
        }
        case kNOT:
        {
            auto num = std::get<int32_t>(operandStack().top());
            operandStack().pop();
            operandStack().push(!num);
            break;
        }
        case kMINUS:
        {
            auto num = std::get<double>(operandStack().top());
            operandStack().pop();
            operandStack().push(-num);
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
                std::cout << std::boolalpha << op << std::endl;
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
        case kTRUE:
        {
            operandStack().push(true);
            break;
        }
        case kFALSE:
        {
            operandStack().push(false);
            break;
        }
        case kJUMP:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp = index;
            break;
        }
        case kJUMP_IF_NOT_TRUE:
        {
            auto pred = std::get<int32_t>(operandStack().top());
            operandStack().pop();
            if (!pred)
            {
                uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
                mIp = index;
            }
            else
            {
                mIp += 4;
            }
            break;
        }
        case kSET_GLOBAL:
        {
            auto value = operandStack().top();
            operandStack().pop();
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;
            if (mGlobals.size() == index)
            {
                mGlobals.push_back(value);
            }
            else
            {
                mGlobals.at(index) = value;
            }
            break;
        }
        case kGET_GLOBAL:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&mCode.instructions[mIp]);
            mIp += 4;
            operandStack().push(mGlobals.at(index));
            break;
        }
        }
    }
}