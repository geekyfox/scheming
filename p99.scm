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
            (get (lambda (x y)
				(grid-get grid x y)))
			(convert (lambda (x y len)
				(list 'vertical x y len))))
	  (detect-slots get convert width height)))


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
                (if (< x max-x)
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
	(scan-yes! 1 1 2)
	acc))

(write (crossword "test/p99a.dat"))
;(crossword "test/p99a.dat")
