; infinite loop
(define (infinite-loop depth)
	(display (list depth 'bla))
	(display #\newline)
	(infinite-loop (+ 1 depth)))

(infinite-loop 0)

; infinite loop rewritten
(define (infinite-loop depth)
	(loop
		(display depth)
		(display #\newline)
		(set! depth (+ 1 depth))))

; infinite loop trampoline
(define (infinite-loop depth)
	(display (list depth 'bla))
	(display #\newline)
	(invoke-later infinite-loop (+ 1 depth)))
