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

class Compiler
{
    using Index = size_t;
    using SymbolTable = std::map<std::string, Index>;
    SymbolTable mSymbolTable{};
    ByteCode mCode{};
    using FuncInfo = std::tuple<Instructions, SymbolTable, std::string>;
    std::optional<FuncInfo> mFunc{};
    auto& instructions()
    {
        return mFunc ? std::get<0>(mFunc.value()) : mCode.instructions;
    }
    std::pair<Index, Scope> getIndex(std::string const& name) const
    {
        if (mFunc)
        {
            auto const& funcInfo = mFunc.value();
            auto const map = std::get<1>(funcInfo);
            auto iter = map.find(name);
            if (iter != map.end())
            {
                return {iter->second, Scope::kLOCAL};
            }
            // first local args, then self
            if (name == std::get<2>(funcInfo))
            {
                return {0, Scope::kFUNCTION_SELF_REF};
            }
        }

        auto iter = mSymbolTable.find(name);
        ASSERT_MSG(iter != mSymbolTable.end(), name);
        auto const idx = iter->second;
        return {idx, Scope::kGLOBAL};
    }
    std::pair<Index, Scope> defineCurrentFunction(std::string const& name)
    {
        ASSERT(mFunc);
        std::get<2>(mFunc.value()) = name;
        auto const scope = Scope::kFUNCTION_SELF_REF;
        return {0, scope};
    }
    std::pair<Index, Scope> define(std::string const& name)
    {
        auto& map = mFunc ? std::get<1>(mFunc.value()) : mSymbolTable;
        auto const idx = map.size();
        map[name] = idx;
        auto const scope = mFunc ? Scope::kLOCAL : Scope::kGLOBAL;
        return {idx, scope};
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