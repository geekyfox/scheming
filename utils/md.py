#!/usr/bin/env python3

import sys

state = 'initial'

for line in sys.stdin:
    if line.startswith('//'):
        if state == 'code':
            print('```')
        state = 'text'
        line = line[2:].lstrip()
        print(line.rstrip())
    else:
        if state == 'text':
            print('```c')
        state = 'code'
        print(line.rstrip())

