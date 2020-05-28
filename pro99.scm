; problem #1
(define (my-last x)
	(if (null? (cdr x))
		x
		(my-last (cdr x))))

(writeln (my-last '(a b c d)))

; problem #2
(define (my-but-last x)
	(if (null? (cddr x))
		x
		(my-but-last (cdr x))))

(writeln (my-but-last '(a b c d)))

; problem #3
(define element-at list-ref)

(writeln (element-at '(a b c d e) 3))

; problem #4
(writeln (length '(a b c d e)))

; problem #5
(writeln (reverse '(a b c d e)))

; problem #6
(define (palindrome? x)
	(eq? x (reverse x)))

(writeln (palindrome? '(a b c d)))
(writeln (palindrome? '(x a m a x)))

; problem #7
(define (my-flatten x)
	(define (impl x acc)
		(cond
			((null? x) acc)
			((pair? x) (impl (cdr x) (impl (car x) acc)))
			(else (cons x acc))))
	(reverse (impl x '())))

(writeln (my-flatten '(a (b (c d) e))))

; problem #8
(define (compress xs)
	(fold-list xs (lambda (acc x)
		(cond
			((null? acc) (list x))
			((eq? x (car acc)) acc)
			(else (cons x acc))))))

(writeln (compress '(a a a a b c c a a d e e e e f)))

; problem #9
(define (pack xs)
	(fold-list xs (lambda (acc x)
		(cond
			((null? acc) (list (list x)))
			((eq? x (caar acc)) (cons (cons x (car acc)) (cdr acc)))
			(else (cons (list x) acc))))))

(writeln (pack '(a a a a b c c a a d e e e e f)))

; problem #10
(define (encode xs)
	(map
		(lambda (x) (list (length x) (car x)))
		(pack xs)))

(writeln (encode '(a a a a b c c a a d e e e e)))

; problem #11
(define (encode-modified xs)
	(map
		(lambda (x) (if (= 1 (car x)) (cadr x) x))
		(encode xs)))

(writeln (encode-modified '(a a a a b c c a a d e e e e)))

; problem #12
(define (prepend n x acc)
	(if (= n 0) acc (prepend (- n 1) x (cons x acc))))

(define (decode xs)
	(fold-list xs (lambda (acc x)
		(cond
			((pair? x) (prepend (car x) (cadr x) acc))
			(else (cons x acc))))))

(writeln (decode '((4 a) b (2 c) (2 a) d (4 e))))

; problem #13
(define (encode-direct xs)
	(fold-list xs (lambda (acc x)
		(cond
			((null? acc)
				(list x))
			((and (pair? (car acc)) (eq? (cadar acc) x))
				(cons (list (+ 1 (caar acc)) x) (cdr acc)))
			((and (symbol? (car acc)) (eq? (car acc) x))
				(cons (list 2 x) (cdr acc)))
			(else
				(cons x acc))))))

(writeln (encode-direct '(a a a a b c c a a d e e e e)))

; problem #14
(define (dupli xs)
	(fold-list xs
		(lambda (acc x) (prepend 2 x acc))))

(writeln (dupli '(a b c c d)))

; problem #15
(define (repli xs n)
	(fold-list xs
		(lambda (acc x) (prepend n x acc))))

(writeln (repli '(a b c) 3))

; problem #16
(define (drop xs n)
	(define (impl ys m acc)
		(cond
			((null? ys)
				(reverse acc))
			((= m n)
				(impl (cdr ys) 1 acc))
			(else
				(impl (cdr ys) (+ m 1) (cons (car ys) acc)))))
	(impl xs 1 '()))

(writeln (drop '(a b c d e f g h i j) 3))

; problem #17
(define (split xs n)
	(define (impl ys m acc)
		(if (= m n)
			(list (reverse acc) ys)
			(impl (cdr ys) (+ m 1) (cons (car ys) acc))))
	(impl xs 0 '()))

(writeln (split '(a b c d e f g h i k) 3))

; problem #18
(define (slice xs i j)
	(define (impl xs j acc)
	  	(if (= j 0)
			(reverse acc)
			(impl (cdr xs) (- j 1) (cons (car xs) acc))))
	(if (= i 1)
		(impl xs j '())
		(slice (cdr xs) (- i 1) (- j 1))))

 (writeln (slice '(a b c d e f g h i k) 3 7))

 ; problem #19
  (define (rotate xs n)
	(define sz (length xs))
	(define (impl m)
		(append
			(slice xs (+ m 1) sz)
			(slice xs 1 m)))
	(cond
		((= sz 0) xs)
		((< n 0) (impl (+ sz n)))
		(else (impl n))))

(writeln (rotate '(a b c d e f g h) 3))
(writeln (rotate '(a b c d e f g h) -2))

; problem #31

(define primes '(2 3 5 7))

(define (next-prime x)
	(if 
		(prime? x)
		x
		(next-prime (+ x 1))))

(define (prime? x)
	(letrec (
			(impl
				(lambda (pms)
					(cond
						((< x (* (car pms) (car pms)))
							#t)
						((= 0 (modulo x (car pms)))
							#f)
						(else
							(impl (cdr (extend-primes! pms))))))))
		(impl primes)))

(define (extend-primes! pms)
	(if
		(null? (cdr pms))
		(set-cdr! pms (list (next-prime (+ 1 (car pms))))))
	pms)

(writeln (prime? 7))
(writeln (prime? 7771))

; problem #32

(define (gcd x y)
	(if (= y 0) x (gcd y (modulo x y))))

(writeln (gcd 36 63))

; problem #33

(define (coprime? x y)
	(= 1 (gcd x y)))

(writeln (coprime? 36 63))
(writeln (coprime? 35 64))

; problem #34
(define (totient-phi-naive x)
	(letrec (
			(impl
				(lambda (n count)
					(cond
						((= n 0) count)
						((coprime? x n) (impl (- n 1) (+ count 1)))
						(else (impl (- n 1) count))))))
		(impl x 0)))

(writeln (totient-phi-naive 10))
(writeln (totient-phi-naive 100))

; problem #35
(define (prime-factors x)
	(define (impl acc n pms)
		(cond
			((= n 1)
				acc)
			((< n (* (car pms) (car pms)))
				(cons n acc))
			((= 0 (modulo n (car pms)))
				(impl (cons (car pms) acc) (/ n (car pms)) pms))
			(else
				(impl acc n (cdr (extend-primes! pms))))))
	(reverse (impl '() x primes)))

(writeln (prime-factors 315))
(writeln (prime-factors 331155))
(writeln (prime-factors 3311555))
