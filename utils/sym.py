#!/usr/bin/env python3

import re
import sys

syntax = {}
ignore = False

for line in sys.stdin:
    m = re.match(r'void (.+)\(void\) /\* syntax: (.+) \*/', line)
    if m:
        syntax[m.group(2)] = m.group(1)
    if line.strip() == 'void setup_syntax(void)':
        ignore = True
        continue
    if ignore and line.strip() == '}':
        ignore = False
        continue
    if not ignore:
        print(line.rstrip())

print('void setup_syntax(void)')
print('{')
print('\tbzero(&SYNTAX_HANDLERS, sizeof(struct dict));')

for name, func in sorted(syntax.items()):
    print(f'\tregister_syntax_handler("{name}", {func});')

print('}')

