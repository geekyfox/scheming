#!/usr/bin/env python3

import re
import sys

if len(sys.argv) != 2:
    print(f'Usage: {sys.argv[0]} <filename>', file=sys.stderr)
    sys.exit(1)

filename = sys.argv[1]
with open(filename) as fh:
    old_content = fh.readlines()

keyword_to_func = {}
regex = re.compile(r'void (.+)\(void\) /\* syntax: (.+) \*/')
for line in old_content:
    m = re.match(r'void (.+)\(void\) /\* syntax: (.+) \*/', line)
    if m:
        keyword_to_func[m.group(2)] = m.group(1)

new_content = []
skip = False

for line in old_content:
    if line.strip() == 'void setup_syntax(void)':
        skip = True
    elif skip and line.strip() == '}':
        skip = False
    elif skip:
        pass
    else:
        new_content.append(line)

new_content.extend([
    'void setup_syntax(void)\n',
    '{\n',
    '\tbzero(&SYNTAX_HANDLERS, sizeof(struct dict));\n',
])
for name, func in sorted(keyword_to_func.items()):
    new_content.append(f'\tregister_syntax_handler("{name}", {func});\n')
new_content.append('}\n')

if old_content != new_content:
    with open(filename, 'w') as fh:
        fh.writelines(new_content)

