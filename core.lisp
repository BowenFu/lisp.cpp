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