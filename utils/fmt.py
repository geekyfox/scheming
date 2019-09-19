#!/usr/bin/env python3

import sys

def spaces_to_tabs(line):
    prefix = ''
    while True:
        if line.startswith('    '):
            prefix += '\t'
            line = line[4:]
        elif line.startswith('\t'):
            prefix += '\t'
            line = line[1:]
        else:
            return prefix + line

for line in sys.stdin:
    line = spaces_to_tabs(line)
    print(line.rstrip())

