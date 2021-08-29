#ifndef LISP_META_H
#define LISP_META_H

#include <cstdlib>
#include <memory>
#include <sstream>
#include <variant>
#include <iostream>
#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#define ASSERT(_) if (!(_)) { throw std::runtime_error{#_}; }
#define FAIL(_) { throw std::runtime_error{#_}; }

class MExpr;
using MExprPtr = std::shared_ptr<MExpr>;

class MExpr
{
public:
    virtual std::string toString() const = 0;
    virtual ~MExpr() = default;
};

class MAtomic : public MExpr
{
    std::string mText;
public:
    MAtomic(std::string const& text)
    : mText{text}
    {
    }
    std::string toString() const override
    {
        return mText;
    }
    std::string get() const
    {
        return mText;
    }
};

class MNil final: public MExpr
{
    MNil() = default;
public:
    static auto const& instance()
    {
        static MExprPtr n{new MNil{}};
        return n;
    }
    std::string toString() const override
    {
        return "()";
    }
};

class MCons final: public MExpr
{
    MExprPtr mCar;
    MExprPtr mCdr;
public:
    MCons(MExprPtr const& car_, MExprPtr const& cdr_)
    : mCar{car_}
    , mCdr{cdr_}
    {
        ASSERT(mCar->toString() != ".");
        ASSERT(mCdr->toString() != ".");
    }
    std::string toString() const override
    {
        std::ostringstream o;
        o << "(" << mCar->toString();
        if (auto cdr = dynamic_cast<MCons*>(mCdr.get()))
        {
            auto cdrStr = mCdr->toString();
            auto cdrStrSize = cdrStr.size();
            o << " " << cdrStr.substr(1U, cdrStrSize - 2);
        }
        else if (auto cdr = dynamic_cast<MNil*>(mCdr.get()))
        {
        }
        else
        {
            o << " . " << mCdr->toString();
        }
        o << ")";
        return o.str();
    }
    auto car() const
    {
        return mCar;
    }
    auto cdr() const
    {
        return mCdr;
    }
};

#endif // LISP_META_H
