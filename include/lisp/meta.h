#ifndef LISP_META_H
#define LISP_META_H

#include <sstream>

#define ASSERT(_) if (!(_)) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ <<") "; throw std::runtime_error{s.str()}; }
#define FAIL_(_) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ <<") "; throw std::runtime_error{s.str()}; }

template <typename... Ts>
void unused(Ts const&...)
{
    return;
}

#endif // LISP_META_H
