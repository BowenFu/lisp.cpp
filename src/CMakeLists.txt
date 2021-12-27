target_include_directories(lisp PUBLIC
  ${PROJECT_SOURCE_DIR}/include)

target_sources(lisp PRIVATE
evaluator.cpp
vm.cpp
compiler.cpp
)

target_compile_options(lisp PRIVATE ${BASE_COMPILE_FLAGS})

set_target_properties(lisp PROPERTIES CXX_EXTENSIONS OFF)
