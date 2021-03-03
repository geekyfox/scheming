BEGIN {
	ORS = ""
	output = ""
 	is_code = 0
	merge = 0
}

function write(s) {
	print s > output
}

function writeln(s) {
	write(s "\n")
	merge = 0
}

match($0, /^## Chapter ([[:digit:]]+)/, cap) {
	chapter_index = int(cap[1])
	print "chapter_" chapter_index ".md\n" > "leanpub/Book.txt"
	output = "leanpub/chapter_" chapter_index ".md"
	if (chapter_index <= 3) {
		writeln("{sample:true}")
	}
	writeln($0)
	next
}

!output {
	next
}

/^```/ {
	is_code = !is_code
	writeln($0)
	next
}

/^>/ {
	writeln("A" $0)
	next
}

is_code {
	writeln($0)
	next
}

{
	gsub(/^[[:space:]]+/, "", $0)
	gsub(/[[:space:]]+$/, "", $0)

	if ($0) {
		if (merge) {
			write(" ")
		}
		write($0)
		merge = 1
	} else {
		if (merge) {
			write("\n")
		}
		writeln()
	}
}
