BEGIN {
	MXLEN = 72
}

FNR < 5 {
	print $0
	next
}

/^\/\/ ##/ {
	flush()
	print $0
	next
}

/^\/\/ [^>]+/ {
	if (accum) {
		accum = accum " "
	}
	accum = accum substr($0, 4)
	next
}

{
	flush()
	print $0
}

function flush() {
	gsub(/^[ ]+/, "", accum)
	while (accum) {
		len = length(accum)

		if (len <= MXLEN) {
			print "// " accum
			accum = ""
			return
		}

		ix = MXLEN + 1
		while (substr(accum, ix, 1) != " ") {
			ix--
		}

		print "// " substr(accum, 1, ix - 1)
		accum = substr(accum, ix + 1)
		gsub(/^[ ]+/, "", accum)
	}
}
