#!/usr/bin/env ruby

def fix_indent(line)
  indent = ''

  while true do
    if line.start_with? "\t"
      indent += "\t"
      line = line[1..-1]
    elsif line.start_with? "    "
      indent += "\t"
      line = line[4..-1]
    else
      return indent + line
    end
  end
end

syntax_to_func = {}
native_to_func = {}
skip = false
skip_empty = true

STDIN.each_line do |line|
  line = fix_indent(line.rstrip)

  next if line.empty? && skip_empty

  m = line.match(%r{^object_t (syntax_.+)\(.+\) // (\S+)$})
  syntax_to_func[m.captures[1]] = m.captures[0] if m

  m = line.match(%r{^object_t syntax_(.+)\(.+\)$})
  syntax_to_func[m.captures[0]] = 'syntax_' + m.captures[0] if m

  m = line.match(%r{^object_t (native_.+)\(.+\) // (\S+)$})
  native_to_func[m.captures[1]] = m.captures[0] if m

  m = line.match(%r{^object_t native_(.+)\(.+\)$})
  native_to_func[m.captures[0]] = 'native_' + m.captures[0] if m  

  skip = true if line == 'void setup_syntax(void)'
  skip = true if line == 'void register_stdlib_functions(void)'

  puts line unless skip

  skip_empty = line.empty? || skip
  skip = false if line == '}'
end

puts 'void setup_syntax(void)'
puts '{'

syntax_to_func.sort.each do |keyword, func|
  puts "\tregister_syntax_handler(\"#{keyword}\", #{func});"
end

puts '}'
puts ''
puts 'void register_stdlib_functions(void)'
puts '{'

native_to_func.sort.each do |keyword, func|
  puts "\tregister_native(\"#{keyword}\", #{func});"
end

puts '}'
