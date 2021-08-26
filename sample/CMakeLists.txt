set(lisp_SAMPLES
loop
)

foreach(sample ${lisp_SAMPLES})
    add_executable(${sample} ${sample}.cpp)
    target_compile_options(${sample} PRIVATE ${BASE_COMPILE_FLAGS})
    target_link_libraries(${sample} PRIVATE lisp)
    set_target_properties(${sample} PROPERTIES CXX_EXTENSIONS OFF)
endforeach()

macro (do_test test_name arg result)
    add_test(${test_name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/loop ${arg})
    set_tests_properties(${test_name}
      PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endmacro (do_test)

do_test(test_1 "1" 1)
do_test(test_lambda "((lambda () 1))" 1)
do_test(test_lambda "((lambda (x) x) 2)" 2)
do_test(test_null1 "(null? nil)" true)
do_test(test_null2 "(null? 2)" false)
do_test(test_pair1 "(pair? (list 1))" true)
do_test(test_pair2 "(pair? nil)" false)
do_test(test_pair3 "(pair? 2)" false)
do_test(test_eq1 "(eq? true true)" true)
do_test(test_eq2 "(eq? true false)" false)
do_test(test_eq3 "(eq? true 2)" false)
# do_test(test_eq4 "(eq? 2 2)" true) // Fix me
do_test(test_eq5 "(eq? 2 3)" false)
do_test(test_eq6 "(eq? 2 nil)" false)
do_test(test_eq7 "(eq? nil nil)" true)
# do_test(test_eq8 "(eq? (list 2) (list 2))" true) // Fix me

do_test(test_define_proc "(define (x) 2) (x)" 2)
do_test(test_cond1 "(define x #t) (cond (x 1) (else 2))" 1)
do_test(test_cond2 "(define x #f) (cond (x 1) (else 2))" 2)

do_test(test_cond3
"
(define lat?
    (lambda (l)
        (cond
            ((null? l) #t)
            ((atom? (car l)) (lat? (cdr l)))
            (else #f))))
(lat? (list 1 2 3))
"
true)