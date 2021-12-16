#ifndef LISP_META_H
#define LISP_META_H

#define ASSERT(_) if (!(_)) { throw std::runtime_error{#_}; }
#define FAIL(_) { throw std::runtime_error{#_}; }

#endif // LISP_META_H
