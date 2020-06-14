#!/usr/bin/env ruby

def fix_indent(line)
  return "\t" + fix_indent(line[1..-1]) if line.start_with? "\t"
  return "\t" + fix_indent(line[4..-1]) if line.start_with? '    '
  line
end

skip = false
skip_empty = true

STDIN.each_line do |line|
  line = fix_indent(line.rstrip)

  next if line.empty? && skip_empty

  skip = true if line == 'void register_stdlib_syntax(void)'
  skip = true if line == 'void register_stdlib_functions(void)'

  puts line unless skip

  skip_empty = line.empty? || skip
  skip = false if line == '}'
end
