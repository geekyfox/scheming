#!/usr/bin/env ruby

syntax_to_func = {}
native_to_func = {}

STDIN.each_line do |line|
  m = line.match(%r{^object_t (syntax_.+)\(.+\) // (\S+)$})
  syntax_to_func[m.captures[1]] = m.captures[0] if m

  m = line.match(%r{^object_t (native_.+)\(.+\) // (\S+)$})
  native_to_func[m.captures[1]] = m.captures[0] if m
end

puts "typedef struct object* object_t;"
puts "typedef struct scope* scope_t;"
puts

syntax_to_func.each_value do |func|
  puts "object_t #{func}(scope_t, object_t);"
end
puts "void register_syntax(const char* name, object_t (*func)(scope_t, object_t));"

puts
puts 'void register_stdlib_syntax(void)'
puts '{'

syntax_to_func.sort.each do |keyword, func|
  puts "\tregister_syntax(\"#{keyword}\", #{func});"
end

puts '}'
puts

native_to_func.each_value do |func|
  puts "object_t #{func}(int, object_t*);"
end
puts "void register_native(const char* name, object_t (*func)(int, object_t*));"

puts
puts 'void register_stdlib_functions(void)'
puts '{'

native_to_func.sort.each do |keyword, func|
  puts "\tregister_native(\"#{keyword}\", #{func});"
end

puts '}'
