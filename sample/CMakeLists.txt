set(lisp_SAMPLES
interpret
compile
)

foreach(sample ${lisp_SAMPLES})
    add_executable(${sample} ${sample}.cpp)
    target_compile_options(${sample} PRIVATE ${BASE_COMPILE_FLAGS})
    target_link_libraries(${sample} PRIVATE lisp)
    set_target_properties(${sample} PROPERTIES CXX_EXTENSIONS OFF)
endforeach()

macro (do_test test_name arg result)
    add_test(${test_name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/interpret ${arg})
    set_tests_properties(${test_name}
      PROPERTIES PASS_REGULAR_EXPRESSION ${result})
    add_test(${test_name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/compile ${arg})
    set_tests_properties(${test_name}
      PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endmacro (do_test)

do_test(test_1 "1" 1)
do_test(test_lambda "((lambda () 1))" 1)
do_test(test_lambda "((lambda (x) x) 2)" 2)
do_test(test_null1 "(null? null)" true)
do_test(test_null2 "(null? 2)" false)
do_test(test_pair1 "(cons? (list 1))" true)
do_test(test_pair2 "(cons? null)" false)
do_test(test_pair3 "(cons? 2)" false)
do_test(test_eq1 "(eq? true true)" true)
do_test(test_eq2 "(eq? true false)" false)
do_test(test_eq3 "(eq? true 2)" false)
do_test(test_eq4 "(eq? 2 2)" true)
do_test(test_eq5 "(eq? 2 3)" false)
do_test(test_eq6 "(eq? 2 null)" false)
do_test(test_eq7 "(eq? null null)" true)
do_test(test_eq8 "(eq? (list 2) (list 2))" true)

do_test(test_define_proc "(define (x) 2) (x)" 2)
do_test(test_cond1 "(define x #t) (cond (x 1) (else 2))" 1)
do_test(test_cond2 "(define x #f) (cond (x (print x) 1) (else 2))" 2)
do_test(test_cond3 "(define x 1) (cond (x (print x) 1) (else 2))" 1)
do_test(test_cond4 "(define x 1) (cond (x => -) (else 2))" -1)

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
true)

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
do_test(test_-2 "(- 334)" -334)
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

do_test(test_quote_num "(quote 2)" 2)
do_test(test_quote_id "(quote x)" "'x")
do_test(test_quote_nil "(quote ())" "()")
do_test(test_quote_list "(quote (1 xy 23))" "(1 'xy 23)")
do_test(test_quote_sym "(quote (quote x))" "('quote 'x)")
do_test(test_quote_quoted_list "(quote (quote (x)))" "\\\\('quote \\\\('x\\\\)\\\\)")

do_test(test_short_quote_num "'2" 2)
do_test(test_short_quote_id "' x" "'x")
do_test(test_short_quote_nil "'()" "()")
do_test(test_short_quote_list "'(1 xy 23)" "(1 'xy 23)")
do_test(test_short_quote_sym "''x" "('quote 'x)")
do_test(test_short_quote_quoted_list "''(x)" "\\\\('quote \\\\('x\\\\)\\\\)")

do_test(test_quote_unquote_sym "(quote (unquote 'x))" "'x")
do_test(test_quasiquote_unquote_lst "`,(+ 1 2)" "3")
do_test(test_quasiquote_lst_unquote_lst "`(,(+ 1 2))" "(3)")
do_test(test_quasiquote_pair_unquote_lst "`(1 . ,(+ 1 2))" "(1 . 3)")

do_test(test_quasiquote_nested_unquote "\`(1 \`,(+ 1 ,(+ 2 3)) 4)" "\\\\(1 \\\\(\'quasiquote \\\\(\'unquote \\\\(\'\\\\+ 1 5\\\\)\\\\)\\\\) 4\\\\)")

do_test(test_macro_and "(define myAnd (macro (x y) (if x y #f))) (myAnd (+ 1 2) (+ 3 4))" "7")
do_test(test_macro_or "(define myOr (macro (x y) (if x x y))) (myOr (+ 1 2) (+ 3 4))" "3")

do_test(test_macro_negate_num "(define negate (macro (x) `(- ,x))) (negate 1)" "1")
do_test(test_macro_negate_add "(define negate (macro (x) `(- ,x))) (negate (+ 1 1))" "-2")
do_test(test_macro_double "(define double (macro (x) `(* ,x 2))) (double (+ 1 1))" "4")

do_test(test_macro_show "(define show (macro (x) `(list ',x))) (show (+ 1 1))" "\\\\(\\\\'\\\\+ 1 1\\\\)")
do_test(test_macro_first "(define first (macro (x) `(car ',x))) (first (+ 1 1))" "'+")
do_test(test_macro_first "(define rest (macro (x) `(cdr ',x))) (rest (+ 1 1))" "(1 1)")

do_test(test_macro_first "(define replaceOp (macro (orig op) `(,op . ,(cdr orig)))) (replaceOp (+ 3 3) *)" "9")

do_test(test_bind_to_param_list "(define mylist (lambda x x)) (mylist 1 2 3)" "\\\\(1 2 3\\\\)")
do_test(test_cons_pred "(cons? (cons 1 2))" true)
do_test(test_list "(list 1 2)" "(1 2)")
do_test(test_list_star "(list* 1 2)" "(1 . 2)")

do_test(test_let "(define x -1)(let ((x 1)(y (- x))) (+ y x))" "2")
do_test(test_named_let "(define (fib n) (let fib-iter ((a 1)(b 0)(count n)) (if (= count 0) b (fib-iter (+ a b) a (- count 1))))) (fib 3)" "2")
do_test(test_let* "(define x -1)(let* ((x 1)(y (- x))) (+ y x))" "0")

do_test(test_quasiquote_nested "`(1 `,(+ 1 ,(+ 2 3)) 4)" "\\\\(1 \\\\(\\\\'quasiquote \\\\(\\\\'unquote \\\\(\\\\'\\\\+ 1 5\\\\)\\\\)\\\\) 4\\\\)")

do_test(test_unquote-splicing-nested "`(1 ```,,@,,@(list (+ 1 2)) 4)" "\\\\(1 \\\\(\\\\'quasiquote \\\\(\\\\'quasiquote \\\\(\\\\'quasiquote \\\\(\\\\'unquote \\\\(\\\\'unquote-splicing \\\\(\\\\'unquote 3\\\\)\\\\)\\\\)\\\\)\\\\)\\\\) 4\\\\)")

do_test(test_unquote-splicing "(let ((x '(1))) `(,@x 1))" "\\\\(1 1\\\\)")
do_test(test_unquote-splicing2 "`(1 2 ,@(list (+ 1 2) (- 5 1)) 5)" "\\\\(1 2 3 4 5\\\\)")

do_test(test_and_1 "(and)" true)
do_test(test_and_2 "(and 2)" "2")
do_test(test_and_3 "(and 1 \"2\")" "\"2\"")
do_test(test_and_4 "(and 1 \"2\" 'x)" "\'x")
do_test(test_and_5 "(and (or 1 \"2\") 'x)" "\'x")

do_test(test_or_1 "(or)" false)
do_test(test_or_2 "(or 1)" "1")
do_test(test_or_3 "(or \"2\" 1)" "\"2\"")
do_test(test_or_4 "(or 'x \"2\" 1)" "\'x")
do_test(test_or_5 "(or (and #f 'x) \"2\" 1)" "2")


do_test(test_recursive_and "(and 1 (and 2 (and 3 4)))" "4")
do_test(test_recursive_and "(or (or 2 (and 3 4) (and 5 6)))" "2")

do_test(test_map "(map - '(1 2 3))" "\\\\(-1 -2 -3\\\\)")

do_test(test_append "(append '(1 2) '(3 4))" "\\\\(1 2 3 4\\\\)")
do_test(test_filter "(filter even '(1 2 3 4))" "\\\\(2 4\\\\)")