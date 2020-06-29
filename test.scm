(define (bla) (and))
(writeln (bla))
(writeln bla)

(define (foo x) (and x))
(writeln (foo 1))
(writeln foo)

(define (bar x y) (and x y))
(writeln (bar #f 1))
(writeln (bar 1 1))
(writeln bar)

(define (baz x y z) (and x y z))
(writeln (baz #f #f 1))
(writeln baz)
