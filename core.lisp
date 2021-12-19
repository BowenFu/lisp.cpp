(define atom?
    (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

(define not
    (lambda (x)
    (if x false true)))

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
                        `(if ,(car first)
                            (begin ,@(cdr first))
                            ,(expand-clauses rest)
                        )
                    )
                )
            )
        )
        (expand-clauses clauses)
    ))