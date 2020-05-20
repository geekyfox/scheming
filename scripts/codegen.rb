#!/usr/bin/env ruby

syntax_to_func = {}
native_to_func = {}
skip = false

STDIN.each_line do |line|
  line = line.rstrip

  m = line.match(%r{^object_t (syntax_.+)\(.+\) // (\S+)$})
  syntax_to_func[m.captures[1]] = m.captures[0] if m

  m = line.match(%r{^object_t syntax_(.+)\(.+\)$})
  syntax_to_func[m.captures[0]] = 'syntax_' + m.captures[0] if m

  m = line.match(%r{^object_t (native_.+)\(.+\) // (\S+)$})
  native_to_func[m.captures[1]] = m.captures[0] if m

  m = line.match(%r{^object_t native_(.+)\(.+\)$})
  native_to_func[m.captures[0]] = 'native_' + m.captures[0] if m  

  skip ||= line.strip == 'void setup_syntax(void)'
  skip ||= line.strip == 'void register_stdlib_functions(void)'

  puts line unless skip

  skip &&= line.strip != '}'
end

puts 'void setup_syntax(void)'
puts '{'

syntax_to_func.sort.each do |keyword, func|
  puts "\tregister_syntax_handler(\"#{keyword}\", #{func});"
end

puts '}'
puts 'void register_stdlib_functions(void)'
puts '{'

native_to_func.sort.each do |keyword, func|
  puts "\tregister_native(\"#{keyword}\", #{func});"
end

print '}'
