#ifndef LISP_META_H
#define LISP_META_H

#include <sstream>

#define ASSERT(_) if (!(_)) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ <<") "; throw std::runtime_error{s.str()}; }
#define ASSERT_MSG(_, msg) if (!(_)) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ << ", " << (msg) << ") "; throw std::runtime_error{s.str()}; }
#define FAIL_(_) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ <<") "; throw std::runtime_error{s.str()}; }

#define FAIL_MSG(_, msg) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ << " : " << msg<< ") "; throw std::runtime_error{s.str()}; }
template <typename... Ts>
void unused(Ts const&...)
{
    return;
}

#endif // LISP_META_H
