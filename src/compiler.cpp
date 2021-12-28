#include "lisp/compiler.h"
#include <array>

auto integerToFourBytes(size_t num) -> std::array<Byte, 4>
{
    std::array<Byte, 4> result;
    result[3] = num & 0xFF;
    unused(num >>= 8);
    result[2] = num & 0xFF;
    unused(num >>= 8);
    result[1] = num & 0xFF;
    unused(num >>= 8);
    result[0] = num & 0xFF;
    return result;
}

void Compiler::compile(ExprPtr const& expr)
{
    auto const exprPtr = expr.get();
    if (auto numPtr = dynamic_cast<Number const*>(exprPtr))
    {
        auto const index = mCode.constantPool.size();
        mCode.constantPool.push_back(numPtr->get());
        mCode.instructions.push_back(kCONST);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            mCode.instructions.push_back(i);
        }
        return;
    }
    if (auto boolPtr = dynamic_cast<Bool const*>(exprPtr))
    {
        mCode.instructions.push_back(kICONST);
        auto bytes = integerToFourBytes(boolPtr->get());
        for (Byte i : bytes)
        {
            mCode.instructions.push_back(i);
        }
        return;
    }
    if (auto defPtr = dynamic_cast<Definition const*>(exprPtr))
    {
        compile(defPtr->mValue);
        mCode.instructions.push_back(kSET_GLOBAL);
        auto index = mSymbolTable.size();
        mSymbolTable.insert({defPtr->mVariableName, {defPtr->mValue, index}});
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            mCode.instructions.push_back(i);
        }
        return;
    }
    if (auto variablePtr = dynamic_cast<Variable const*>(exprPtr))
    {
        auto const name = variablePtr->name();
        auto index = mSymbolTable.at(name).second;
        mCode.instructions.push_back(kGET_GLOBAL);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            mCode.instructions.push_back(i);
        }
        return;
    }
    if (auto ifPtr = dynamic_cast<If const*>(exprPtr))
    {
        compile(ifPtr->mPredicate);
        mCode.instructions.push_back(kJUMP_IF_NOT_TRUE);
        // jump to alternative
        auto const jump0OperandIndex = mCode.instructions.size();
        for (size_t i = 0; i < 4; ++i)
        {
            mCode.instructions.push_back(0);
        }
        compile(ifPtr->mConsequent);
        mCode.instructions.push_back(kJUMP);
        // jump to post alternative
        auto const jump1OperandIndex = mCode.instructions.size();
        for (size_t i = 0; i < 4; ++i)
        {
            mCode.instructions.push_back(0);
        }
        // update jump0
        {
            auto const alterPos = mCode.instructions.size();
            auto bytes = integerToFourBytes(alterPos);
            for (size_t i = 0; i < 4; ++i)
            {
                mCode.instructions.at(jump0OperandIndex + i) = bytes[i];
            }
        }
        compile(ifPtr->mAlternative);
        // update jump1
        {
            auto const postAlterPos = mCode.instructions.size();
            auto bytes = integerToFourBytes(postAlterPos);
            for (size_t i = 0; i < 4; ++i)
            {
                mCode.instructions.at(jump1OperandIndex + i) = bytes[i];
            }
        }
        return;
    }
    if (auto appPtr = dynamic_cast<Application const*>(exprPtr))
    {
        auto nbOperands = appPtr->mOperands.size();
        OpCode opCode = [appPtr, nbOperands]
        {
            auto opName = appPtr->mOperator->toString();
            if (opName == "+")
            {
                return kADD;
            }
            else if (opName == "-")
            {
                ASSERT(nbOperands == 2U || nbOperands == 1U);
                return nbOperands == 2U ? kSUB : kMINUS;
            }
            else if (opName == "*")
            {
                return kMUL;
            }
            else if (opName == "/")
            {
                ASSERT(nbOperands == 2U);
                return kDIV;
            }
            else if (opName == "==")
            {
                ASSERT(nbOperands == 2U);
                return kEQUAL;
            }
            else if (opName == "!=")
            {
                ASSERT(nbOperands == 2U);
                return kNOT_EQUAL;
            }
            else if (opName == ">")
            {
                ASSERT(nbOperands == 2U);
                return kGREATER_THAN;
            }
            else if (opName == "!")
            {
                ASSERT(nbOperands == 1U);
                return kNOT;
            }
            FAIL_("Not supported yet!");
        }();
        if (nbOperands == 1)
        {
            compile(appPtr->mOperands.at(0));
            mCode.instructions.push_back(opCode);
            return;
        }
        compile(appPtr->mOperands.at(0));
        for (auto i = 1U; i < nbOperands; ++i)
        {
            compile(appPtr->mOperands.at(i));
            mCode.instructions.push_back(opCode);
        }
        return;
    }
    FAIL_("Not supported yet!");
}
