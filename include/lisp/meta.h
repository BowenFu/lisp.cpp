#ifndef LISP_META_H
#define LISP_META_H

#define ASSERT(_) if (!(_)) { std::stringstream s; s << __FILE__ << ":" << __LINE__ << " (" << #_ <<") "; throw std::runtime_error{s.str()}; }
#define FAIL(_) { throw std::runtime_error{#_}; }

#endif // LISP_META_H
