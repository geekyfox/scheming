(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
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
