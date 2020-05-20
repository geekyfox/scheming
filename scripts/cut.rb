#!/usr/bin/env ruby

STDIN.each_line do |line|
  break if line.strip == '// CUTOFF'
  puts line
end
