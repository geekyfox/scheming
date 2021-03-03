FNR == 1 {
	prev = $0
	next
}

(prev == "``` c") && ($0 == "") {
	next
}

(prev == "") && ($0 == "```") {
	prev = $0
	next
}

{
	print prev
	prev = $0
}

ENDFILE {
	print prev
}