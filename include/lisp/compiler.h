#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"
#include <optional>

enum class Scope
{
    kLOCAL,
    kGLOBAL,
    kFUNCTION_SELF_REF
};

class CurrentFunctionIndex
{};

inline constexpr CurrentFunctionIndex currentFunctionIndex{};

using VarInfo = std::pair<size_t, Scope>;
class SymbolTable
{
    std::map<std::string, VarInfo> mNameToVarInfo{};
    SymbolTable const* mEnclosing{};
public:
    SymbolTable() = default;
    SymbolTable(SymbolTable const* const enclosing)
    : mEnclosing{enclosing}
    {}
    auto extend() const
    {
        return SymbolTable(this);
    }
    std::optional<VarInfo> resolve(std::string const& name)
    {
        auto const* symTable = this;
        do
        {
            auto const& map = symTable->mNameToVarInfo;
            auto const iter = map.find(name);
            if (iter != map.end())
            {
                return iter->second;
            }
            symTable = symTable->mEnclosing;
        } while (symTable != nullptr);
        return {};
    }
    VarInfo define(std::string const& name, Scope scope)
    {
        auto const index = mNameToVarInfo.size();
        auto const varInfo = VarInfo{index, scope};
        mNameToVarInfo[name] = varInfo;
        return varInfo;
    }
};

class Compiler
{
    SymbolTable mSymbolTable{};
    ByteCode mCode{};
    using FuncInfo = std::tuple<Instructions, SymbolTable, std::string>;
    std::stack<FuncInfo> mFuncStack{};
    auto& instructions()
    {
        return mFuncStack.empty() ? mCode.instructions : std::get<0>(mFuncStack.top());
    }
    auto& symbolTable()
    {
        return mFuncStack.empty() ? mSymbolTable : std::get<1>(mFuncStack.top());
    }
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
        // first local args, then self
        if (name == std::get<2>(funcInfo))
        {
            return {0, Scope::kFUNCTION_SELF_REF};
        }
        FAIL_("Resolve failed!");
    }
    VarInfo defineCurrentFunction(std::string const& name)
    {
        ASSERT(!mFuncStack.empty());
        std::get<2>(mFuncStack.top()) = name;
        auto const scope = Scope::kFUNCTION_SELF_REF;
        return {0, scope};
    }
    VarInfo define(std::string const& name)
    {
        auto const scope = mFuncStack.empty() ? Scope::kGLOBAL : Scope::kLOCAL;
        return symbolTable().define(name, scope);
    }
    void emitIndex(size_t index);
public:
    Compiler() = default;
    void compile(ExprPtr const& expr);
    ByteCode code() const
    {
        return mCode;
    }
};

#endif // LISP_COMPILER_H