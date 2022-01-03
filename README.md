# lisp.cpp

### lisp.cpp : a simple lisp interpreter / compiler in C++ with scheme style syntax + quote / unquote style macros.

The interpreter is inspired by SICP.

The macro system and the compiler is inspired by `Writing an interpreter / a compiler in Go`.

### Interpreter sample

```bash
$ build/bin/interpret

;;; M-Eval input:
(define x '(2 3))

;;; M-Eval value:
(2 3)

;;; M-Eval input:
`(,@x 1)

;;; M-Eval value:
(2 3 1)

;;; M-Eval input:

```

### Compiler sample

```bash
$ build/bin/compile "(define x '(2 3)) \`(,@x 1)" 
(2 3 1)
```

Refer to `core.lisp` and `sample/CMakeLists.txt` for more samples.

All macros are defined in `core.lisp`.

<!-- ![lisp](./lisp.svg) -->

![Standard](https://img.shields.io/badge/c%2B%2B-17/20-blue.svg)
![Type](https://img.shields.io/badge/type-macro--free-blue)

![Platform](https://img.shields.io/badge/platform-linux-blue)
![Platform](https://img.shields.io/badge/platform-osx-blue)
![Platform](https://img.shields.io/badge/platform-win-blue)

[![CMake](https://github.com/BowenFu/lisp.cpp/actions/workflows/cmake.yml/badge.svg)](https://github.com/BowenFu/lisp.cpp/actions/workflows/cmake.yml)
[![CMake](https://github.com/BowenFu/lisp.cpp/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/BowenFu/lisp.cpp/actions/workflows/sanitizers.yml)
![GitHub license](https://img.shields.io/github/license/BowenFu/lisp.cpp.svg)
[![codecov](https://codecov.io/gh/BowenFu/lisp.cpp/branch/main/graph/badge.svg?token=BNWHFLNNSD)](https://codecov.io/gh/BowenFu/lisp.cpp)
