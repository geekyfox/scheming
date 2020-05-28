(define (crossword filename)
	(let* (
			(data (read-crossword filename))
			(words (car data))
			(grid (cdr data))
			(v-slots (detect-v-slots grid)))
	v-slots))

(define (read-crossword filename)
	(letrec (
			(f (lambda (acc lines)
				(if
					(string=? (car lines) "")
					(cons (reverse acc) (cdr lines))
					(f (cons (car lines) acc) (cdr lines))))))
		(f '() (read-lines (open-input-file filename)))))

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
			(dot? (lambda (col row)
				(eq? (grid-get grid col row) #\.)))
			(save (lambda (acc col st-row row)
				(if
					(= 1 (- row st-row))
					acc
					(cons
						(list 'vertical col st-row (- row st-row))
						acc))))
			(f (lambda (acc col)
				(if
					(> col width)
					acc
					(f (g acc col) (+ col 1)))))
			(g (lambda (acc col)
				(if
					(dot? col 1)
					(h acc col 1 2)
					(k acc col 2))))
			(h (lambda (acc col st-row row)
				(cond
					((> row height)
						(save acc col st-row row))
					((dot? col row)
						(h acc col st-row (+ 1 row)))
					(else
						(k 
							(save acc col st-row row)
							col (+ 1 row))))))
			(k (lambda (acc col row)
				(cond
					((> row height)
						acc)
					((dot? col row)
						(h acc col row (+ 1 row)))
					(else
						(k acc col (+ 1 row)))))))
	;(dot? 1 2)))
	;(g '() 1)))
	(f '() 1)))


(write (crossword "test/p99a.dat"))
;(crossword "test/p99a.dat")
