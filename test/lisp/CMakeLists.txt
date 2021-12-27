add_executable(unittests
test.cpp
testVm.cpp
testCompiler.cpp
)
target_include_directories(unittests PRIVATE
  ${PROJECT_SOURCE_DIR}/src)
target_compile_options(unittests PRIVATE ${BASE_COMPILE_FLAGS})
target_link_libraries(unittests PRIVATE lisp gtest_main)
set_target_properties(unittests PROPERTIES CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS OFF)
gtest_discover_tests(unittests)