#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

#include "vm.h"
#include "evaluator.h"
#include <optional>

enum class Scope
{
    kLOCAL,
    kGLOBAL
};

class Compiler
{
    using SymbolTable = std::map<std::string, size_t>;
    SymbolTable mSymbolTable{};
    ByteCode mCode{};
    using FuncInfo = std::pair<Instructions, SymbolTable>;
    std::optional<FuncInfo> mFunc{};
    auto& instructions()
    {
        return mFunc ? mFunc.value().first : mCode.instructions;
    }
    std::pair<size_t, Scope> getIndex(std::string const& name) const
    {
        if (mFunc)
        {
            auto const map = mFunc.value().second;
            auto iter = map.find(name);
            if (iter != map.end())
            {
                return {iter->second, Scope::kLOCAL};
            }
        }
        auto const idx = mSymbolTable.at(name);
        return {idx, Scope::kGLOBAL};
    }
    Scope define(std::string const& name)
    {
        auto& map = mFunc ? mFunc.value().second : mSymbolTable;
        auto const idx = map.size();
        map[name] = idx;
        return mFunc ? Scope::kLOCAL : Scope::kGLOBAL;
    }
public:
    Compiler() = default;
    void compile(ExprPtr const& expr);
    ByteCode code() const
    {
        return mCode;
    }
};

#endif // LISP_COMPILER_H