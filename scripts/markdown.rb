#!/usr/bin/env ruby

require 'English'

def detect_language(filename)
  return 'scheme' if filename.end_with? '.scm'
  return 'c' if filename.end_with? '.c'
  return ''
end

def comment_prefix(language)
  return ';' if language == 'scheme'
  return '//' if language == 'c'
  return '#'
end

def embed(filename, section)
  flag = false

  language = detect_language(filename)
  comment = comment_prefix(language)

  puts "```#{language}"
  File.open(filename).each_line do |line|
    if line.start_with? comment
      flag = line[(comment.length)..-1].strip == section
    elsif flag
      puts line.rstrip
    end
  end
  puts '```'
end

state = :text
STDIN.each_line do |line|
  line = line.rstrip
  if line =~ %r{// embed (.*) : (.*)}
    puts '```' if state == :code
    embed($LAST_MATCH_INFO[1], $LAST_MATCH_INFO[2])
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

if state == :code
  puts '```'
end
