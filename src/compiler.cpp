#include "lisp/compiler.h"

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
        // todo: refactor
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            mCode.instructions.push_back(i);
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
                ASSERT(nbOperands == 2U);
                return kSUB;
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
            FAIL_("Not supported yet!");
        }();
        compile(appPtr->mOperands.at(0));
        for (auto i = 1U; i < nbOperands; ++i)
        {
            compile(appPtr->mOperands.at(i));
            mCode.instructions.push_back(opCode);
        }
        return;
    }
}
