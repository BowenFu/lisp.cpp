#include "lisp/vm.h"
#include "lisp/meta.h"
#include <iostream>
#include <cmath>

namespace vm{
void print(std::ostream& o, StackFrame const& f)
{
    o << "StackFrame " << f.closure()->funcSym().name();
}

void print(std::ostream& o, FunctionSymbol const& f)
{
    o << "Function " << f.name();
}

void print(std::ostream& o, ClosurePtr const& c)
{
    o << "Closure " << c->funcSym().name();
}

void print(std::ostream& o, VMNull)
{
    o << "null";
}

constexpr bool operator== (ConsPtr const& lhs, ConsPtr const& rhs);

bool operator== (FunctionSymbol const& lhs, FunctionSymbol const& rhs)
{
    return lhs.name() == rhs.name() &&
           lhs.nbArgs() == rhs.nbArgs() &&
           lhs.variadic() == rhs.variadic() &&
           lhs.nbLocals() == rhs.nbLocals() &&
           lhs.instructions() == rhs.instructions();
}

template <typename T>
constexpr bool operator== (Literal<T> const& lhs, Literal<T> const& rhs)
{
    return lhs.value == rhs.value;
}

bool operator== (Symbol const& lhs, Symbol const& rhs)
{
    return lhs.value == rhs.value;
}

bool operator== (Splicing const& lhs, Splicing const& rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator== (VMNull const&, VMNull const&)
{
    return true;
}

constexpr bool operator== (ConsPtr const& lhs, ConsPtr const& rhs)
{
    if (lhs.get() == rhs.get())
    {
        return true;
    }
    if (!lhs || !rhs)
    {
        return false;
    }
    return lhs->car() == rhs->car() && lhs->cdr() == rhs->cdr();
}

template <typename T>
void print(std::ostream& o, Literal<T> const& obj)
{
    o << std::boolalpha << obj.value;
}

void print(std::ostream& o, Symbol const& obj)
{
    o << obj.value;
}

void print(std::ostream& o, String const& obj)
{
    o << "\"" << obj.value << "\"";
}

std::ostream& operator << (std::ostream& o, Object const& obj);

std::string VMCons::toString() const
{
    std::ostringstream o;
    o << "(" << mCar;
    if (auto const consPtrPtr = std::get_if<ConsPtr>(&mCdr))
    {
        auto cdrStr = (*consPtrPtr)->toString();
        auto cdrStrSize = cdrStr.size();
        o << " " << cdrStr.substr(1U, cdrStrSize - 2);
    }
    else if (std::get_if<VMNull>(&mCdr))
    {
    }
    else
    {
        o << " . " << mCdr;
    }
    o << ")";
    return o.str();
}

void print(std::ostream& o, vm::ConsPtr const& cons_)
{
    ASSERT(cons_);
    o << cons_->toString();
}

void print(std::ostream& o, vm::Splicing const& spl)
{
    print(o, spl.value);
}

std::ostream& operator << (std::ostream& o, Object const& obj)
{
    std::visit([&o](auto op)
    {
        print(o, op);
    }, obj);
    return o;
}

auto deCons(Object const& obj)
{
    auto consPtrPtr = std::get_if<ConsPtr>(&obj);
    ASSERT(consPtrPtr);
    ASSERT(consPtrPtr->get());
    return std::make_pair(consPtrPtr->get()->car(), consPtrPtr->get()->cdr());
}

std::vector<Object> consToVec(ConsPtr const& cons_)
{
    std::vector<Object> vec;
    Object me = cons_;
    while (!std::get_if<VMNull>(&me))
    {
        auto [car, cdr] = deCons(me);
        vec.push_back(car);
        me = cdr;
    }
    return vec;
}

Object vecToCons(std::vector<Object> const& vec)
{
    Object result = vmNull;
    auto vecSize = vec.size();
    auto i = vec.rbegin();
    if (vecSize >= 2)
    {
        auto dot = vec.at(vecSize - 2);
        auto dotPtr = std::get_if<Symbol>(&dot);
        if (dotPtr != nullptr && dotPtr->value == ".")
        {
            ASSERT(vecSize >=3);
            ++i;
            ++i;
            result = vec.back();
        }
    }
    for (;i != vec.rend(); ++i)
    {
        result = cons(*i, result);
    }
    return result;
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
            operandStack().push(Int{word});
            break;
        }
        case kIADD:
        {
            auto const rhs = std::get<Int>(operandStack().top());
            operandStack().pop();
            auto const lhs = std::get<Int>(operandStack().top());
            operandStack().pop();
            int32_t result = lhs.value + rhs.value;
            operandStack().push(Int{result});
            break;
        }
        case kEQUAL:
        {
            auto const rhs = operandStack().top();
            operandStack().pop();
            auto const lhs = operandStack().top();
            operandStack().pop();
            bool result = lhs == rhs;
            operandStack().push(Bool{result});
            break;
        }
        case kADD:
        case kSUB:
        case kMUL:
        case kDIV:
        case kMOD:
        case kLESS_THAN:
        {
            auto const rhs = operandStack().top();
            operandStack().pop();
            auto const lhs = operandStack().top();
            operandStack().pop();
            if (auto lhsDPtr = std::get_if<Double>(&lhs))
            {
                auto lhsD = *lhsDPtr;
                auto rhsD = std::get<Double>(rhs);
                switch (opCode)
                {
                case kADD:
                {
                    double result = lhsD.value + rhsD.value;
                    operandStack().push(Double{result});
                    break;
                }
                
                case kSUB:
                {
                    double result = lhsD.value - rhsD.value;
                    operandStack().push(Double{result});
                    break;
                }
                
                case kMUL:
                {
                    double result = lhsD.value * rhsD.value;
                    operandStack().push(Double{result});
                    break;
                }
                
                case kDIV:
                {
                    double result = lhsD.value / rhsD.value;
                    operandStack().push(Double{result});
                    break;
                }
                
                case kMOD:
                {
                    ASSERT(std::trunc(lhsD.value) == lhsD.value);
                    ASSERT(std::trunc(rhsD.value) == rhsD.value);
                    double result = static_cast<int32_t>(lhsD.value) % static_cast<int32_t>(rhsD.value);
                    operandStack().push(Double{result});
                    break;
                }
                
                case kLESS_THAN:
                {
                    bool result = lhsD.value < rhsD.value;
                    operandStack().push(Bool{result});
                    break;
                }

                default:
                    FAIL_("Unsupported op");
                }
            }
            else if (auto lhsStrPtr = std::get_if<String>(&lhs))
            {
                auto lhsStr = *lhsStrPtr;
                auto rhsStr = std::get<String>(rhs);
                switch (opCode)
                {
                case kADD:
                {
                    auto result = lhsStr.value + rhsStr.value;
                    operandStack().push(String{result});
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
            auto value = std::get<Bool>(operandStack().top());
            operandStack().pop();
            operandStack().push(Bool{!value.value});
            break;
        }
        case kMINUS:
        {
            auto num = std::get<Double>(operandStack().top());
            operandStack().pop();
            operandStack().push(Double{-num.value});
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
        case kERROR:
        {
            auto op = operandStack().top();
            operandStack().pop();
            std::cout << "Error : " << op << std::endl;
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
            auto const closurePtr = *closurePtrPtr;
            auto const functionSymbol = closurePtr->funcSym();
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
                Object rest = vmNull;
                for (size_t i = 0; i < nbRest; ++i)
                {
                    rest = cons(operandStack().top(), rest);
                    operandStack().pop();
                }
                params.at(nbArgs - 1) = rest;
                for (size_t i = nbArgs - 1; i > 0; --i)
                {
                    params.at(i - 1) = operandStack().top();
                    operandStack().pop();
                }
            }

            mCallStack.push(StackFrame{closurePtr, std::move(params), mIp});
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
            operandStack().push(Bool{true});
            break;
        }
        case kFALSE:
        {
            operandStack().push(Bool{false});
            break;
        }
        case kNULL:
        {
            operandStack().push(vmNull);
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
            auto predPtr = std::get_if<Bool>(&operandStack().top());
            operandStack().pop();
            // not false => true
            auto const isTrue = (predPtr == nullptr || predPtr->value);
            if (!isTrue)
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
        case kSPLICING:
        {
            auto const op = operandStack().top();
            operandStack().pop();
            operandStack().push(Splicing{std::get<ConsPtr>(op)});
            break;
        }
        case kCONS:
        {
            auto const cdr = operandStack().top();
            operandStack().pop();
            auto const car = operandStack().top();
            operandStack().pop();
            if (auto const splicingPtr = std::get_if<Splicing>(&car))
            {
                auto vec = consToVec(splicingPtr->value); 
                vec.push_back(Symbol{"."});
                vec.push_back(cdr);
                operandStack().push(vecToCons(vec));
            }
            else
            {
                operandStack().push(cons(car, cdr));
            }
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
        case kIS_CONS:
        {
            auto const obj = operandStack().top();
            operandStack().pop();
            auto const consPtrPtr = std::get_if<ConsPtr>(&obj);
            operandStack().push(Bool{consPtrPtr != nullptr});
            break;
        }
        case kIS_NULL:
        {
            auto const obj = operandStack().top();
            operandStack().pop();
            auto const nilPtr = std::get_if<VMNull>(&obj);
            operandStack().push(Bool{nilPtr != nullptr});
            break;
        }
        }
    }
}
} // namespace vm