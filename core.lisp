(define atom?
    (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

(define not
    (lambda (x)
    (if x false true)))

(define
    (- . lst)
        (cond
            ((null? (cdr lst)) (* -1 (car lst)))
            (else (+ (car lst) (* -1 (car (cdr lst)))))
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

(define let (macro (param-argu-pairs body)
    (define recur (lambda (param-argu-pairs-internal)
        (if (cons? param-argu-pairs-internal)
            `((lambda (,(caar param-argu-pairs-internal)) ,(recur (cdr param-argu-pairs-internal))) ,(cadar param-argu-pairs-internal))
            `,body
        )
    ))
    (recur param-argu-pairs)
    )
)

(define mycond
  (macro clauses
    (define (cond-clauses->if lst)
        (if (atom? lst)
            #f
            (let ((clause (car lst)))
            (if (or (eq? (car clause) 'else)
                    (eq? (car clause) #t))
                (if (null? (cdr clause))
                    (car clause)
                    (cons 'begin (cdr clause)))
                (if (null? (cdr clause))
                    (list 'or
                            (car clause)
                            (cond-clauses->if (cdr lst)))
                    (if (eq? (cadr clause) '=>)
                        (if (1arg-lambda? (caddr clause))
                            (let ((var (caadr (caddr clause))))
                                `(let ((,var ,(car clause)))
                                (if ,var ,(cons 'begin (cddr (caddr clause)))
                                    ,(cond-clauses->if (cdr lst)))))
                            (let ((b (gensym)))
                                `(let ((,b ,(car clause)))
                                (if ,b
                                    (,(caddr clause) ,b)
                                    ,(cond-clauses->if (cdr lst))))))
                        (list 'if
                                (car clause)
                                (cons 'begin (cdr clause))
                                (cond-clauses->if (cdr lst)))))))))
  (cond-clauses->if clauses)))
