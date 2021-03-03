function startswith(s, prefix) {
        return prefix == substr(s, 0, length(prefix))
}

function detect_language(filename) {
        if (match(filename, /\.scm$/)) {
                return "scheme"
        }
        if (match(filename, /\.c$/)) {
                return "c"
        }
        if (match(filename, /\.awk$/)) {
                return "awk"
        }
        return ""
}

function comment_prefix(language) {
        if (language == "scheme") {
                return ";"
        }
        if (language == "c") {
                return "//"
        }
        return "#"
}

function embed(filename, section) {
        language = detect_language(filename)
        comment = comment_prefix(language)
        marker = comment " " section
        flag = 0

        print "```" language

        while ((getline line < filename) > 0) {
                if (line == marker) {
                        flag = 1
                } else if (startswith(line, comment)) {
                        flag = 0
                } else if (flag) {
                        print line
                }
        }
        close(filename)

        print "```"
}

function embed_full(filename) {
        language = detect_language(filename)
        comment = comment_prefix(language)

        print "```" language

        while ((getline line < filename) > 0) {
                print line
        }
        close(filename)

        print "```"
}

BEGINFILE {
        state = "text"

        print "# Scheming: The journey through functional programming, language design, insobriety and bad jokes"
        print "[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)"
}

function switch_to_text() {
        if (state == "code") {
                print "```"
                state = "text"
        }
}

function switch_to_code() {
        if (state == "text") {
                print "``` c"
                state = "code"
        }
}

match($0, /^\/\/ embed (.*) : (.*)/, cap) {
        switch_to_text()
        embed(cap[1], cap[2])
        next
}

match($0, /^\/\/ embed (.*)/, cap) {
        switch_to_text()
        embed_full(cap[1])
        next
}

/^\/\// {
        switch_to_text()
        print substr($0, 4)
        next
}

{
        switch_to_code()
        print $0
}

ENDFILE {
        switch_to_text()

        print "This chapter is a work in progress for now and will be"
        print "published once it'll be ready."

        print ""

        print "To make it happen a bit sooner, you can send some encouraging"
        print "words to ivan.appel@gmail.com, or you can also send some"
        print "encouraging money using the \"Donate\" button below, or both."

        print ""

        print "[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/donate?hosted_button_id=5GNRHRET3USG2)"
}

