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

; problem #99
(define (crossword filename)
	(crossword-solve (list (read-crossword filename))))

(define (crossword-solve puzzles)
	(cond
		((null? puzzles)
			"No solution")
		((not (car puzzles))
			(crossword-solve (cdr puzzles)))
		((null? (caar puzzles))
			(list-ref (car puzzles) 3))
		(else
			(crossword-solve
				(append
					(fork (car puzzles))
					(cdr puzzles))))))

(define (fork puzzle)
	(let* (
			(slots (list-ref puzzle 1))
			(slot (car slots))
			(slot-length (list-ref slot 4))
			(words (list-ref puzzle 2))
			(grid (list-ref puzzle 3))
			(selected-words (select-words slot-length words))
			(inject (lambda (ws)
				(let* (
						(new-grid (fit grid slot (car ws))))
					(if
						new-grid
						(list (cdr slots) (cdr ws) new-grid)
						#f)))))
		(map inject selected-words)))

(define (fit grid slot word)
	(let* (
			(dir (list-ref slot 1))
			(col (list-ref slot 2))
			(row (list-ref slot 3))
			(letters (string->list word)))
		(if
			(eq? dir 'vertical)
			(fit-vertical grid letters col row)
			(fit-horizontal grid letters col row))))

(define (fit-vertical grid letters x y)
	(cond
		((null? letters) 
			grid)
		((eq? (grid-get grid x y) (car letters))
			(fit-vertical
				grid
				(cdr letters)
				x
				(+ y 1)))
		((eq? (grid-get grid x y) #\.)
			(fit-vertical
				(grid-set grid x y (car letters))
				(cdr letters)
				x
				(+ y 1)))
		(else
			#f)))

(define (fit-horizontal grid letters x y)
	(cond
		((null? letters) 
			grid)
		((eq? (grid-get grid x y) #\.)
			(fit-horizontal
				(grid-set grid x y (car letters))
				(cdr letters)
				(+ x 1)
				y))
		(else
			#f)))


(define (grid-set grid x y letter) 
	(if (= y 1)
		(cons
			(string-set (car grid) x letter)
			(cdr grid))
		(cons
			(car grid)
			(grid-set (cdr grid) x (- y 1) letter))))

(define (string-set str ix ch)
	(let* (
			(copy (string-copy str)))
		(string-set! copy ix ch)
		copy))

(define (select-words len words)
	(letrec (
			(acc '())
			(push! (lambda (v)
				(set! acc (cons v acc))))
			(save! (lambda (ws backlog)
				(push! (append ws backlog))))
			(scan! (lambda (ws backlog)
				(cond
					((null? ws)
						'())
					((= len (string-length (car ws)))
						(save! ws backlog)
						(scan! (cdr ws) (cons (car ws) backlog)))
					(else
						(scan! (cdr ws) (cons (car ws) backlog)))))))
		(scan! words '())
		acc))

(define (read-crossword-data filename)
	(letrec (
			(f (lambda (acc lines)
				(if
					(string=? (car lines) "")
					(cons (reverse acc) (cdr lines))
					(f (cons (car lines) acc) (cdr lines))))))
		(f '() (read-lines (open-input-file filename)))))

(define (read-crossword filename)
	(let* (
			(data (read-crossword-data filename))
			(words (car data))
			(grid (cdr data))
			(v-slots (detect-v-slots grid))
			(h-slots (detect-h-slots grid))
			(slots (append v-slots h-slots)))
		(list (reverse slots) words grid)))

(define (read-lines port)
	(letrec (
			(f (lambda (acc) (g acc (read-line port))))
			(g (lambda (acc line)
				(if
					(eof-object? line)
					(reverse acc)
					(f (cons line acc))))))
		(f '())))

(define (read-line port)
	(letrec (
			(f (lambda (acc) (g acc (read-char port))))
			(g (lambda (acc ch)
				(if
					(or (eof-object? ch) (eq? ch #\newline))
					(h ch acc)
					(f (cons ch acc)))))
			(h (lambda (ch acc)
				(if
					(and (eof-object? ch) (null? acc))
					ch
					(list->string (reverse acc))))))
		(f '())))

(define (grid-get grid col row)
	(if
		(= row 1)
		(string-ref (car grid) (- col 1))
		(grid-get (cdr grid) col (- row 1))))

(define (detect-v-slots grid)
	(letrec (
			(height (length grid))
			(width (string-length (car grid)))
            (get (lambda (x y)
				(grid-get grid x y)))
			(convert (lambda (x y len)
				(list 'vertical x y len))))
	  (detect-slots get convert width height)))


(define (detect-h-slots grid)
	(letrec (
			(height (length grid))
			(width (string-length (car grid)))
            (get (lambda (x y)
				(grid-get grid y x)))
			(convert (lambda (x y len)
				(list 'horizontal y x len))))
	  (detect-slots get convert height width)))


(define (detect-slots get convert x-max y-max)
	(letrec (
			(dot? (lambda (x y)
				(eq? (get x y) #\.)))
			(acc '())
			(push! (lambda (item)
				(set! acc (cons item acc))))
			(save! (lambda (x y-start y-end)
				(if (> y-end y-start)
				  (push! (convert x y-start (+ 1 (- y-end y-start)))))))
			(scan! (lambda (x)
                (scan-col! x)
                (if (< x x-max)
                  (scan! (+ x 1)))))
            (scan-col! (lambda (x)
				(if
					(dot? x 1)
					(scan-yes! x 1 2)
					(scan-no! x 2))))
            (scan-yes! (lambda (x y-yes y-scan)
                (cond
                    ((> y-scan y-max)
                        (save! x y-yes y-max))
                    ((dot? x y-scan)
                        (scan-yes! x y-yes (+ y-scan 1)))
                    (else
                        (save! x y-yes (- y-scan 1))
                        (scan-no! x (+ y-scan 1))))))
            (scan-no! (lambda (x y-scan)
                (cond
					((> y-scan y-max)
						'())
					((dot? x y-scan)
						(scan-yes! x y-scan (+ y-scan 1)))
					(else
						(scan-no! x (+ y-scan 1)))))))
	(scan! 1)
	acc))

(define (display-grid grid)
	(cond
		((not (null? grid))
			(display (car grid))
			(newline)
			(display-grid (cdr grid)))))

(display-grid (crossword "test/p99a.dat"))
