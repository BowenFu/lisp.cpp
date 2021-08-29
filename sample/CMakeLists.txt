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
do_test(test_eq4 "(eq? 2 2)" true)
do_test(test_eq5 "(eq? 2 3)" false)
do_test(test_eq6 "(eq? 2 nil)" false)
do_test(test_eq7 "(eq? nil nil)" true)
do_test(test_eq8 "(eq? (list 2) (list 2))" true)

do_test(test_define_proc "(define (x) 2) (x)" 2)
do_test(test_cond1 "(define x #t) (cond (x 1) (else 2))" 1)
do_test(test_cond2 "(define x #f) (cond (x 1) (else 2))" 2)

do_test(test_cond3
"
(define lat?  (lambda (l) (cond ((null? l) #t) ((atom? (car l)) (lat? (cdr l))) (else #f)))) (lat? (list 1 2 (list 3)))
"
false)

do_test(test_string1
"
(define member?
    (lambda (a lat)
        (cond
            ((null? lat) #f)
            (else (or (eq? (car lat) a)
                        (member? a (cdr lat))
            )))))
(member? \"abc\" (list 1 \"abcd\"))
"
false)

do_test(test_string2 " (eq? \"abc\" \"abc\") " true)

do_test(test_string3
"
(eq? \"abc\" \"abcd\")
"
false)

do_test(test_string4
"
(define member?
    (lambda (a lat)
        (cond
            ((null? lat) #f)
            (else (or (eq? (car lat) a)
                        (member? a (cdr lat))
            )))))
(member? \"abc\" (list 1 \"abc\"))
"
false)

do_test(test_remember
"
(define rember
    (lambda (a lat)
        (cond
            ((null? lat) lat)
            ((eq? a (car lat)) (cdr lat))
            (else (cons (car lat) (rember a (cdr lat)))))))
(rember \"abc\" (list 1 \"abc\"))
"
"\(1\)")

do_test(test_+ "(+ 137 349)" 486)
do_test(test_+2 "(+ 2.7 10)" 12.7)
do_test(test_+3 "(+ 21 35 12 7)" 75)
do_test(test_- "(- 1000 334)" 666)
do_test(test_* "(* 5 99)" 495)
do_test(test_*2 "(* 25 4 12)" 1200)
do_test(test_/ "(/ 10 5)" 2)
do_test(test_nested "(+ (* 3 5) (- 10 6))" 19)

do_test(test_cond4
"
(define (abs x)
    (cond ((< x 0) (- x))
        (else x))
)
(abs -2)
" 2)
