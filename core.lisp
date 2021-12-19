(define (len lst)
    ; (if (atom? lst) (error "Not a list when calling with len" lst) '())
    (if (null? lst)
        0
        (+ 1 (len (cdr lst)))
    )
)
(define let (macro name-arg-param-pairs-body
    (define arg-num (len name-arg-param-pairs-body))
    (if (= arg-num 2)
        (begin
          (define arg-param-pairs (car name-arg-param-pairs-body))
          (define body (cdr name-arg-param-pairs-body))
          (define let->combination (lambda (arg-param-pairs-internal)
            (define args (map car arg-param-pairs-internal))
            (define params (map cadr arg-param-pairs-internal))
            `((lambda ,args (begin ,@body)) ,@params)))
            (let->combination arg-param-pairs))
        (begin
          (define name (car name-arg-param-pairs-body))
          (define arg-param-pairs (cadr name-arg-param-pairs-body))
          (define body (cddr name-arg-param-pairs-body))
          (define let->combination (lambda (arg-param-pairs-internal)
            (define args (map car arg-param-pairs-internal))
            (define params (map cadr arg-param-pairs-internal))
            `(begin (define ,name (lambda ,args (begin ,@body))) (,name ,@params))))
            (let->combination arg-param-pairs)))
    )
)

; cannot pass function call results to macro call.
(define let* (macro (arg-param-pairs body)
    (define recur (lambda (arg-param-pairs-internal)
        (if (cons? arg-param-pairs-internal)
            `((lambda (,(caar arg-param-pairs-internal)) ,(recur (cdr arg-param-pairs-internal))) ,(cadar arg-param-pairs-internal))
            body
        )
    ))
    (recur arg-param-pairs)
    )
)

(define and
    (macro lst
        (define (impl ls)
            (if (null? ls)
                #t
                (if (cons? ls)
                    (if (null? (cdr ls))
                        (car ls)
                        `(if ,(car ls)
                            ,(impl (cdr ls))
                            #f
                        )
                    )
                    #f
                )
            )
        )
        (impl lst)
    )
)

(define or
    (macro lst
        (define (impl ls)
            (if (null? ls)
                #f
                (if (cons? ls)
                    (if (null? (cdr ls))
                        (car ls)
                        `(if ,(car ls)
                            ,(car ls)
                            ,(impl (cdr ls))
                        )
                    )
                    #f
                )
            )
        )
        (impl lst)
    )
)

(define cond
    (macro clauses
        (define (expand-clauses clauses)
            (if (null? clauses)
                'false
                (begin
                    ; not sure why let does not work well inside macros
                    (define first (car clauses))
                    (define rest (cdr clauses))
                    (if (eq? (car first) 'else)
                        (if (null? rest)
                            `(begin ,@(cdr first))
                            (error "ELSE clause isn't last -- COND->IF" clauses)
                        )
                        (if (null? (cddr first))
                            `(if ,(car first)
                                (begin ,@(cdr first))
                                ,(expand-clauses rest)
                            )
                            (if (eq? '=> (cadr first))
                                `(begin
                                    (define check-result ,(car first))
                                    (if check-result
                                        (,(caddr first) check-result)
                                        ,(expand-clauses rest)
                                    )
                                )
                                `(if ,(car first)
                                    (begin ,@(cdr first))
                                    ,(expand-clauses rest)
                                )
                            )
                        )
                    )
                )
            )
        )
        (expand-clauses clauses)
    ))

(define atom?
    (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

(define not
    (lambda (x)
    (if x false true)))

(define >
    (lambda (x y)
        (< y x)
    )
)

(define <=
    (lambda (x y)
        (not (> x y))
    )
)

(define >=
    (lambda (x y)
        (not (< x y))
    )
)

(define
    (- . lst)
        (define rest (cdr lst))
        (if
            (cons? rest) (+ (car lst) (* -1 (car rest)))
            (* -1 (car lst))
        ))

(define list (lambda args args))

(define list*
       (lambda args
        (define $f
        (lambda (xs)
            (if (cons? xs)
                (if (cons? (cdr xs))
                    (cons (car xs) ($f (cdr xs)))
                    (car xs))
                nil)))
        ($f args)
       ))

(define append
    (lambda (lhs rhs)
        (if (null? lhs)
            rhs
            (cons (car lhs) (append (cdr lhs) rhs))
        )
    )
)

(define (caar args)
    (car (car args))
)

(define (cadr args)
    (car (cdr args))
)

(define (cdar args)
    (cdr (car args))
)

(define (cddr args)
    (cdr (cdr args))
)

(define (caddr args)
    (car (cdr (cdr args)))
)

(define (cadar args)
    (car (cdr (car args)))
)

(define (caadr args)
    (car (car (cdr args)))
)

(define (map proc lst)
    (if (null? lst)
        '()
        (cons (proc (car lst)) (map proc (cdr lst)))
    )
)

; not sure why let* does not work in this case.
; fix let* later
(define (filter pred lst)
    (if (null? lst)
        '()
        (let*
           ((elem (car lst))
            (check-result (pred elem))
            (rest-result (filter pred (cdr lst))))
                (if check-result
                    (cons elem rest-result)
                    rest-result)
        )
    )
)

(define (even num) (= (% num 2) 0)) 
