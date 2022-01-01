#include "lisp/vm.h"
#include "lisp/meta.h"
#include <iostream>

std::ostream& operator << (std::ostream& o, StackFrame const& f)
{
    return o << "StackFrame " << f.closure()->funcSym().name();
}

std::ostream& operator << (std::ostream& o, FunctionSymbol const& f)
{
    return o << "Function " << f.name();
}

std::ostream& operator << (std::ostream& o, ClosurePtr const& c)
{
    return o << "Closure " << c->funcSym().name();
}

std::ostream& operator << (std::ostream& o, VMNil)
{
    return o << "nil";
}

std::ostream& operator << (std::ostream& o, Object const& obj);

namespace std
{
std::ostream& operator << (std::ostream& o, ConsPtr const& cons_)
{
    o << "(";
    o << car(cons_);
    o << " . ";
    o << cdr(cons_);
    o << ")";
    return o;
}
}

std::ostream& operator << (std::ostream& o, Object const& obj)
{
    std::visit([&o](auto op)
    {
        o << std::boolalpha << op;
    }, obj);
    return o;
}


void VM::run()
{
    while (mIp < instructions().size())
    {
        Byte opCode = instructions()[mIp];
        ++mIp;
        switch (opCode)
        {
        case kICONST:
        {
            int32_t word = fourBytesToInteger<int32_t>(&instructions()[mIp]);
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
                    FAIL_("Unsupported op");
                }
            }
            else if (auto lhsStrPtr = std::get_if<std::string>(&lhs))
            {
                auto lhsStr = *lhsStrPtr;
                auto rhsStr = std::get<std::string>(rhs);
                switch (opCode)
                {
                case kADD:
                {
                    auto result = lhsStr + rhsStr;
                    operandStack().push(result);
                    break;
                }
                
                case kEQUAL:
                {
                    bool result = lhsStr == rhsStr;
                    operandStack().push(result);
                    break;
                }

                case kNOT_EQUAL:
                {
                    bool result = lhsStr != rhsStr;
                    operandStack().push(result);
                    break;
                }

                default:
                    FAIL_("Unsupported op");
                }
            }
            else
            {
                FAIL_("Unsupported operand type!");
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
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;
            operandStack().push(mCode.constantPool.at(index));
            break;
        }
        case kPRINT:
        {
            auto op = operandStack().top();
            operandStack().pop();
            std::cout << op << std::endl;
            break;
        }
        case kHALT:
            return;
        case kCALL:
        {
            uint32_t nbParams = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;

            auto const closurePtrPtr = std::get_if<ClosurePtr>(&operandStack().top());
            ASSERT(closurePtrPtr);
            auto const functionSymbol = (*closurePtrPtr)->funcSym();
            operandStack().pop();
            auto const nbArgs = functionSymbol.nbArgs();
            ASSERT(nbParams + 1 >= nbArgs);
            std::vector<Object> params(nbArgs + functionSymbol.nbLocals());
            if (!functionSymbol.variadic())
            {
                ASSERT(nbParams == nbArgs);
                for (size_t i = nbArgs; i > 0; --i)
                {
                    params.at(i - 1) = operandStack().top();
                    operandStack().pop();
                }
            }
            else
            {
                auto nbRest = nbParams + 1 - nbArgs; 
                Object rest = vmNil;
                for (size_t i = 0; i < nbRest; ++i)
                {
                    rest = cons(operandStack().top(), rest);
                    operandStack().pop();
                }
                params.back() = rest;
                for (size_t i = nbArgs - 1; i > 0; --i)
                {
                    params.at(i - 1) = operandStack().top();
                    operandStack().pop();
                }
            }

            mCallStack.push(StackFrame{*closurePtrPtr, std::move(params), mIp});
            mIp = 0;
            break;
        }
        case kRET:
        {
            mIp = mCallStack.top().returnAddress();
            mCallStack.pop();
            break;
        }
        case kGET_LOCAL:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;

            operandStack().push(mCallStack.top().locals(index));
            break;
        }
        case kSET_LOCAL:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
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
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp = index;
            break;
        }
        case kJUMP_IF_NOT_TRUE:
        {
            auto pred = std::get<int32_t>(operandStack().top());
            operandStack().pop();
            if (!pred)
            {
                uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
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
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
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
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;
            operandStack().push(mGlobals.at(index));
            break;
        }
        case kPOP:
        {
            operandStack().pop();
            break;
        }
        case kCONS:
        {
            auto const rhs = operandStack().top();
            operandStack().pop();
            auto const lhs = operandStack().top();
            operandStack().pop();
            operandStack().push(cons(lhs, rhs));
            break;
        }
        case kCAR:
        case kCDR:
        {
            auto const obj = operandStack().top();
            auto const consPtrPtr = std::get_if<ConsPtr>(&obj);
            ASSERT(consPtrPtr);
            auto const& consPtr = *consPtrPtr;
            operandStack().pop();
            operandStack().push(opCode == kCAR ? car(consPtr) : cdr(consPtr));
            break;
        }
        case kCURRENT_FUNCTION:
        {
            operandStack().push(mCallStack.top().closure());
            break;
        }
        case kCLOSURE:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;
            uint32_t nbFreeVars = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;
            auto freeVars = std::vector<Object>(nbFreeVars);
            for (size_t i = nbFreeVars; i > 0; --i)
            {
                freeVars[i-1] = operandStack().top();
                operandStack().pop();
            }
            auto const funcSym = mCode.constantPool.at(index);
            auto const closurePtr = std::make_shared<Closure>(std::get<FunctionSymbol>(funcSym), freeVars);
            operandStack().push(closurePtr);
            break;
        }
        case kGET_FREE:
        {
            uint32_t index = fourBytesToInteger<uint32_t>(&instructions()[mIp]);
            mIp += 4;

            operandStack().push(mCallStack.top().closure()->freeVars().at(index));
            break;
        }
        }
    }
}