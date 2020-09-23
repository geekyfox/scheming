#!/usr/bin/env ruby

require 'English'

output = nil
is_code = false
merge = false

STDIN.each_line do |line|
  if line =~ %r{^## Chapter (\d+)}
    output.close if output
    chapter_index = $LAST_MATCH_INFO[1]
    filename = "leanpub/chapter_#{chapter_index}.md"
    output = File.open(filename, 'w')
    output.write("{sample:true}\n") if chapter_index.to_i <= 3
    output.write(line)
    merge = false
    next
  end

  next unless output

  if line.start_with? '```'
    is_code = !is_code
    output.write(line)
    merge = false
    next
  end

  if line.start_with? '>'
    output.write("\n") if merge
    output.write('A' + line)
    merge = false
    next
  end
 
  if is_code
    output.write(line)
    merge = false
    next
  end

  line = line.strip
  if line.empty?
    output.write("\n") if merge
    output.write("\n")
    merge = false
  else
    output.write(' ') if merge
    output.write(line)
    merge = true
  end
end

output.close unless output.nil?
