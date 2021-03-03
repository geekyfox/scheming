function flush() {
	if (accum && chapter) {
		filename = "temp/grammar_" chapter ".txt"
		print accum > filename
		print "" > filename
	}
	accum = ""
}

BEGIN {
	code = 0
	chapter = 0
	accum = ""
}

/^##/ {
	flush()

	chapter++ # = count + 1
	gsub("## ", "", $0)
	accum = $0
	next
}

/^```/ {
	code = !code
	next
}

code {
	next
}

/^(\s)*$/ {
	flush()
	next
}

/^\* / {
	flush()
}

{
	if (accum) {
		accum = accum " "
	}
	gsub("`", "\"", $0)
	gsub("\"+", "\"", $0)
	accum = accum $0
}

END {
	flush()
}
