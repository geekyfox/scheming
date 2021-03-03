#!/usr/bin/awk -f

match($0, /undefined reference to `(.+)'/, cap) {
    gaps[cap[1]] = cap[1]
}

END {
    asort(gaps)
    for (ix in gaps) {
        print gaps[ix]
    }
}

