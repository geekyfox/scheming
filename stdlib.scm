(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))
(define (cadar x) (car (cdar x)))
	
; (define (fold f x xs)
;	(if (null? xs) x (fold f (f x (car xs)) (cdr xs))))

(define (fold-list xs f)
	(reverse (fold f '() xs)))
	
;(define (length xs)
;	(fold (lambda (ct x) (+ ct 1)) 0 xs))

(define (append xs ys)
	(reverse
		(fold
			(lambda (acc x) (cons x acc))
			(reverse xs)
			ys)))

; (define (reverse xs)
;	(fold (lambda (tail x) (cons x tail)) '() xs))
	
(define (map f xs)
	(fold-list xs
		(lambda (acc x) (cons (f x) acc))))

(define (writeln x)
	(write x)
	(newline))

(define eof-object? null?)

(define (> a b) (< b a))

(define-syntax and
	(syntax-rules ()
		((and)
			#t)
		((and test)
			test)
		((and test1 test2 ...)
			(if test1 (and test2 ...) #f))))
