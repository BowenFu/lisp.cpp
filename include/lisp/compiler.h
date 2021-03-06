#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"
#include <optional>

enum class Scope
{
    kLOCAL,
    kGLOBAL,
    kFUNCTION_SELF_REF,
    kFREE
};

class CurrentFunctionIndex
{};

inline constexpr CurrentFunctionIndex currentFunctionIndex{};

using VarInfo = std::pair<size_t, Scope>;
class SymbolTable
{
    std::map<std::string, VarInfo> mNameToVarInfo{};
    std::vector<VarInfo> mOrigFreeVars{};
    size_t mNbDefinitions{};
    SymbolTable* mEnclosing{};
public:
    SymbolTable() = default;
    SymbolTable(SymbolTable* const enclosing)
    : mEnclosing{enclosing}
    {}
    auto extend()
    {
        return SymbolTable(this);
    }
    auto defineFreeVar(std::string const& name, VarInfo const& orig)
    {
        auto freeIndex = mOrigFreeVars.size();
        mOrigFreeVars.push_back(orig);
        auto const freeVar = VarInfo{freeIndex, Scope::kFREE};
        mNameToVarInfo[name] = freeVar;
        return freeVar;
    }
    std::optional<VarInfo> resolve(std::string const& name)
    {
        // found
        auto const& map = mNameToVarInfo;
        auto const iter = map.find(name);
        if (iter != map.end())
        {
            return iter->second;
        }

        // enclosing
        if (mEnclosing)
        {
            auto const opVarInfo = mEnclosing->resolve(name);
            if (opVarInfo.has_value())
            {
                auto const varInfo = opVarInfo.value();
                if (varInfo.second == Scope::kGLOBAL)
                {
                    return varInfo;
                }
                return defineFreeVar(name, varInfo);
            }
        }

        return {};
    }

    VarInfo define(std::string const& name, Scope scope)
    {
        auto const index = mNbDefinitions;
        auto const varInfo = VarInfo{index, scope};
        mNameToVarInfo[name] = varInfo;
        ++mNbDefinitions;
        return varInfo;
    }
    auto freeVariables() const
    {
        return mOrigFreeVars;
    }
    auto nbDefinitions() const
    {
        return mNbDefinitions;
    }
    VarInfo defineCurrentFunction(std::string const& name)
    {
        auto const varInfo = VarInfo{0, Scope::kFUNCTION_SELF_REF};
        mNameToVarInfo[name] = varInfo;
        return varInfo;
    }
};

class Compiler
{
    SymbolTable mSymbolTable{};
    vm::ByteCode mCode{};
    using FuncInfo = std::tuple<vm::Instructions, SymbolTable>;
    std::stack<FuncInfo> mFuncStack{};
    auto& instructions()
    {
        return mFuncStack.empty() ? mCode.instructions : std::get<0>(mFuncStack.top());
    }
    auto& symbolTable()
    {
        return mFuncStack.empty() ? mSymbolTable : std::get<1>(mFuncStack.top());
    }
    void emitVar(VarInfo const& varInfo);
    void emitIndex(size_t index);
    void emitApplication(Application const& app);
    VarInfo resolve(std::string const& name)
    {
        if (mFuncStack.empty())
        {
            auto varInfo = mSymbolTable.resolve(name);
            ASSERT_MSG(varInfo, name);
            return varInfo.value();
        }
        
        auto& funcInfo = mFuncStack.top();
        auto& symTable = std::get<1>(funcInfo);
        if (auto varInfo = symTable.resolve(name))
        {
            return varInfo.value();
        }
        FAIL_MSG("Resolve failed!", name);
    }
    VarInfo defineCurrentFunction(std::string const& name)
    {
        ASSERT(!mFuncStack.empty());
        return symbolTable().defineCurrentFunction(name);
    }
    VarInfo define(std::string const& name)
    {
        auto const scope = mFuncStack.empty() ? Scope::kGLOBAL : Scope::kLOCAL;
        return symbolTable().define(name, scope);
    }
public:
    Compiler() = default;
    void compile(ExprPtr const& expr);
    vm::ByteCode code() const
    {
        return mCode;
    }
};

#endif // LISP_COMPILER_H