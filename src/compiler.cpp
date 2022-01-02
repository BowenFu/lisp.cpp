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

void Compiler::emitIndex(size_t index)
{
    auto bytes = integerToFourBytes(index);
    for (Byte i : bytes)
    {
        instructions().push_back(i);
    }
}

void Compiler::emitVar(VarInfo const& varInfo)
{
    switch (varInfo.second)
    {
    case Scope::kGLOBAL:
        instructions().push_back(kGET_GLOBAL);
        emitIndex(varInfo.first);
        break;
    
    case Scope::kLOCAL:
        instructions().push_back(kGET_LOCAL);
        emitIndex(varInfo.first);
        break;
    
    case Scope::kFREE:
        instructions().push_back(kGET_FREE);
        emitIndex(varInfo.first);
        break;

    case Scope::kFUNCTION_SELF_REF:
        FAIL_("Never reach here!");
    }
}

void Compiler::emitApplication(Application const& app)
{
    auto nbOperands = app.mOperands.size();
    auto const emitUnaryOp = [&app, this, nbOperands](OpCode opCode)
    {
        ASSERT (nbOperands == 1)
        compile(app.mOperands.at(0));
        instructions().push_back(static_cast<OpCode>(opCode));
    };
    auto const emitBinaryOps = [&app, this, nbOperands](OpCode opCode)
    {
        compile(app.mOperands.at(0));
        for (auto i = 1U; i < nbOperands; ++i)
        {
            compile(app.mOperands.at(i));
            instructions().push_back(opCode);
        }
    };
    bool isPrimitive = true;
    // primitive procedure
    {
        auto opName = app.mOperator->toString();
        if (opName == "+")
        {
            emitBinaryOps(kADD);
        }
        else if (opName == "-")
        {
            ASSERT(nbOperands == 2U || nbOperands == 1U);
            if (nbOperands == 2U)
            {
                emitBinaryOps(kSUB);
            }
            else
            {
                emitUnaryOp(kMINUS);
            }
        }
        else if (opName == "*")
        {
            emitBinaryOps(kMUL);
        }
        else if (opName == "/")
        {
            ASSERT(nbOperands == 2U);
            emitBinaryOps(kDIV);
        }
        else if (opName == "=")
        {
            ASSERT(nbOperands == 2U);
            emitBinaryOps(kEQUAL);
        }
        else if (opName == "!=")
        {
            ASSERT(nbOperands == 2U);
            emitBinaryOps(kNOT_EQUAL);
        }
        else if (opName == ">")
        {
            ASSERT(nbOperands == 2U);
            emitBinaryOps(kGREATER_THAN);
        }
        else if (opName == "!")
        {
            ASSERT(nbOperands == 1U);
            emitUnaryOp(kNOT);
        }
        else if (opName == "cons")
        {
            ASSERT(nbOperands == 2U);
            emitBinaryOps(kCONS);
        }
        else if (opName == "car")
        {
            ASSERT(nbOperands == 1U);
            emitUnaryOp(kCAR);
        }
        else if (opName == "cdr")
        {
            ASSERT(nbOperands == 1U);
            emitUnaryOp(kCDR);
        }
        else if (opName == "show")
        {
            ASSERT(nbOperands == 1U);
            emitUnaryOp(kPRINT);
        }
        else
        {
            isPrimitive = false;
        }
    }
    // lambda procedure.
    if (isPrimitive == false)
    {
        for (auto const &o : app.mOperands)
        {
            compile(o);
        }
        compile(app.mOperator);
        instructions().push_back(kCALL);
        emitIndex(app.mOperands.size());
    }
}

void Compiler::compile(ExprPtr const& expr)
{
    auto const exprPtr = expr.get();
    if (auto numPtr = dynamic_cast<Number const*>(exprPtr))
    {
        auto const index = mCode.constantPool.size();
        mCode.constantPool.push_back(numPtr->get());
        instructions().push_back(kCONST);
        emitIndex(index);
        return;
    }
    if (auto symPtr = dynamic_cast<Symbol const*>(exprPtr))
    {
        auto const index = mCode.constantPool.size();
        mCode.constantPool.push_back(symPtr->toString());
        instructions().push_back(kCONST);
        emitIndex(index);
        return;
    }
    if (auto strPtr = dynamic_cast<String const*>(exprPtr))
    {
        auto const index = mCode.constantPool.size();
        mCode.constantPool.push_back(strPtr->get());
        instructions().push_back(kCONST);
        emitIndex(index);
        return;
    }
    if (auto boolPtr = dynamic_cast<Bool const*>(exprPtr))
    {
        instructions().push_back(kICONST);
        emitIndex(boolPtr->get());
        return;
    }
    if (auto defPtr = dynamic_cast<Definition const*>(exprPtr))
    {
        if (auto lambdaPtr = dynamic_cast<LambdaBase<CompoundProcedure>*>(defPtr->mValue.get()))
        {
            lambdaPtr->setName(defPtr->mVariableName);
        }
        compile(defPtr->mValue);
        auto [index, scope] = define(defPtr->mVariableName);
        ASSERT (scope != Scope::kFUNCTION_SELF_REF);
        auto setIns = scope == Scope::kLOCAL ? kSET_LOCAL : kSET_GLOBAL;
        instructions().push_back(setIns);
        emitIndex(index);
        return;
    }
    if (auto variablePtr = dynamic_cast<Variable const*>(exprPtr))
    {
        auto const name = variablePtr->name();
        auto const varInfo = resolve(name);
        if (varInfo.second == Scope::kFUNCTION_SELF_REF)
        {
            instructions().push_back(kCURRENT_FUNCTION);
            return;
        }
        emitVar(varInfo);
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
            // todo, some actions push stack, some do not. Those pushing stack should pop their values.
        }
        return;
    }
    if (auto lambdaPtr = dynamic_cast<LambdaBase<CompoundProcedure> const*>(exprPtr))
    {
        FuncInfo funcInfo{};
        std::get<1>(funcInfo) = symbolTable().extend();
        mFuncStack.push(funcInfo);
        if (!lambdaPtr->mName.empty())
        {
            defineCurrentFunction(lambdaPtr->mName);
        }
        auto const& [args, variadic] = lambdaPtr->mArguments;
        for (auto const& arg : args)
        {
            auto [_, scope] = define(arg);
            ASSERT(scope == Scope::kLOCAL);
        }
        compile(lambdaPtr->mBody);
        instructions().push_back(kRET);
        auto funcInstructions = std::get<0>(mFuncStack.top());
        auto const freeVars = symbolTable().freeVariables();
        auto const nbLocals = symbolTable().nbDefinitions() - args.size();
        mFuncStack.pop();
        for (auto f : freeVars)
        {
            emitVar(f);
        }
        auto const index = mCode.constantPool.size();
        auto const funcSym = FunctionSymbol{lambdaPtr->mName, args.size(), variadic, nbLocals, funcInstructions};
        mCode.constantPool.push_back(funcSym);
        instructions().push_back(kCLOSURE);
        emitIndex(index);
        emitIndex(freeVars.size());
        return;
    }
    if (auto appPtr = dynamic_cast<Application const*>(exprPtr))
    {
        emitApplication(*appPtr);
        return;
    }
    FAIL_("Not supported yet!");
}
