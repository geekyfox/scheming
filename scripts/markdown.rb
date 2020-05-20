#!/usr/bin/env ruby

def embed(filename, section)
  flag = false

  puts '```scheme'
  File.open(filename).each_line do |line|
    if line.start_with? ';'
      flag = line[1..-1].strip == section
    end
    if flag
      puts line
    end
  end
  puts '```'
end

state = :text
STDIN.each_line do |line|
  line = line.rstrip
  # m = line.match(%r{// embed (.*) : (.*)})
  if line =~ %r{// embed (.*) : (.*)}
    puts '```' if state == :code    
    embed($~[1], $~[2])
    state = :text
  elsif line.start_with? '//'
    if state == :code
      puts '```'
      state = :text
    end
    puts line[2..-1].strip
  else
    if state == :text
      puts '``` c'
      state = :code
    end
    puts line
  end
end
