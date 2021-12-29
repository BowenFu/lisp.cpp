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
        instructions().push_back(kCONST);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto strPtr = dynamic_cast<String const*>(exprPtr))
    {
        auto const index = mCode.constantPool.size();
        mCode.constantPool.push_back(strPtr->get());
        instructions().push_back(kCONST);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto boolPtr = dynamic_cast<Bool const*>(exprPtr))
    {
        instructions().push_back(kICONST);
        auto bytes = integerToFourBytes(boolPtr->get());
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto defPtr = dynamic_cast<Definition const*>(exprPtr))
    {
        compile(defPtr->mValue);
        if (auto* funcSymPtr = std::get_if<FunctionSymbol>(&mCode.constantPool.back()))
        {
            funcSymPtr->setName(defPtr->mVariableName);
        }
        auto [index, scope] = define(defPtr->mVariableName);
        auto setIns = scope == Scope::kLOCAL ? kSET_LOCAL : kSET_GLOBAL;
        instructions().push_back(setIns);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto variablePtr = dynamic_cast<Variable const*>(exprPtr))
    {
        auto const name = variablePtr->name();
        auto [index, scope] = getIndex(name);
        auto getIns = scope == Scope::kLOCAL ? kGET_LOCAL : kGET_GLOBAL;
        instructions().push_back(getIns);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto ifPtr = dynamic_cast<If const*>(exprPtr))
    {
        compile(ifPtr->mPredicate);
        instructions().push_back(kJUMP_IF_NOT_TRUE);
        // jump to alternative
        auto const jump0OperandIndex = instructions().size();
        for (size_t i = 0; i < 4; ++i)
        {
            instructions().push_back(0);
        }
        compile(ifPtr->mConsequent);
        instructions().push_back(kJUMP);
        // jump to post alternative
        auto const jump1OperandIndex = instructions().size();
        for (size_t i = 0; i < 4; ++i)
        {
            instructions().push_back(0);
        }
        // update jump0
        {
            auto const alterPos = instructions().size();
            auto bytes = integerToFourBytes(alterPos);
            for (size_t i = 0; i < 4; ++i)
            {
                instructions().at(jump0OperandIndex + i) = bytes[i];
            }
        }
        compile(ifPtr->mAlternative);
        // update jump1
        {
            auto const postAlterPos = instructions().size();
            auto bytes = integerToFourBytes(postAlterPos);
            for (size_t i = 0; i < 4; ++i)
            {
                instructions().at(jump1OperandIndex + i) = bytes[i];
            }
        }
        return;
    }
    if (auto seqPtr = dynamic_cast<Sequence const*>(exprPtr))
    {
        for (auto const& e : seqPtr->mActions)
        {
            compile(e);
            instructions().push_back(kPOP);
        }
        instructions().resize(instructions().size() - 1);
        return;
    }
    if (auto lambdaPtr = dynamic_cast<LambdaBase<CompoundProcedure> const*>(exprPtr))
    {
        // FIXME: find a place for function def.
        mFunc = FuncInfo{};
        auto const& args = lambdaPtr->mArguments.first;
        for (auto const& arg : args)
        {
            auto [idx, scope] = define(arg);
            ASSERT(scope == Scope::kLOCAL);
        }
        compile(lambdaPtr->mBody);
        instructions().push_back(kRET);
        auto funcInstructions = mFunc.value().first;
        mFunc = {};
        auto const index = mCode.constantPool.size();
        auto const funcSym = FunctionSymbol{args.size(), /* nbLocals= */ 0, funcInstructions};
        mCode.constantPool.push_back(funcSym);
        instructions().push_back(kCONST);
        auto bytes = integerToFourBytes(index);
        for (Byte i : bytes)
        {
            instructions().push_back(i);
        }
        return;
    }
    if (auto appPtr = dynamic_cast<Application const*>(exprPtr))
    {
        auto nbOperands = appPtr->mOperands.size();
        int32_t opCode = [appPtr, nbOperands] () -> int32_t
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
            return -1;
        }();
        // primitive procedure
        if (opCode >= 0)
        {
            if (nbOperands == 1)
            {
                compile(appPtr->mOperands.at(0));
                instructions().push_back(static_cast<OpCode>(opCode));
                return;
            }
            compile(appPtr->mOperands.at(0));
            for (auto i = 1U; i < nbOperands; ++i)
            {
                compile(appPtr->mOperands.at(i));
                instructions().push_back(static_cast<OpCode>(opCode));
            }
            return;
        }
        // lambda procedure.
        {
            for (auto const& o : appPtr->mOperands)
            {
                compile(o);
            }
            compile(appPtr->mOperator);
            instructions().push_back(kCALL);
            auto bytes = integerToFourBytes(appPtr->mOperands.size());
            for (Byte i : bytes)
            {
                instructions().push_back(i);
            }
        }
        return;
    }
    FAIL_("Not supported yet!");
}
