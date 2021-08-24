set(lisp_SAMPLES
loop
)

foreach(sample ${lisp_SAMPLES})
    add_executable(${sample} ${sample}.cpp)
    target_compile_options(${sample} PRIVATE ${BASE_COMPILE_FLAGS})
    target_link_libraries(${sample} PRIVATE lisp)
    set_target_properties(${sample} PROPERTIES CXX_EXTENSIONS OFF)
endforeach()