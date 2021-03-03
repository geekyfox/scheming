match($0, /^object_t (syntax_.+)\(.+\) \/\/ ([^[:space:]]+)$/, cap) {
    syntax_to_fun[cap[2]] = cap[1]
}

match($0, /^object_t (native_.+)\(.+\) \/\/ ([^[:space:]]+)$/, cap) {
    native_to_fun[cap[2]] = cap[1]
}

/\/\/ CODE BELOW IS AUTOGENERATED/ {
    codegen()
    exit(0)
}

{
    print $0
}

ENDFILE {
    codegen()
}

function codegen() {
    print "// CODE BELOW IS AUTOGENERATED"
    print "void register_builtins(void)"
    print "{"

    for (syntax in syntax_to_fun) {
        fun = syntax_to_fun[syntax]
        print "\tregister_syntax(\"" syntax "\", " fun ");"
    }

    for (native in native_to_fun) {
        fun = native_to_fun[native]
        print "\tregister_native(\"" native "\", " fun ");"
    }
    
    print "}"
}
