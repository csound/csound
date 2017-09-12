#!/usr/bin/env python

import os, fnmatch

SRC_DIR="/Users/stevenyi/Desktop/Csound Catalog/"
print SRC_DIR

matches = []
for root, dirnames, filenames in os.walk(SRC_DIR):
  for filename in fnmatch.filter(filenames, '*.orc'):
      matches.append(os.path.join(root, filename))

csoundCommand = "csound -+msg_color=0 --new-parser --syntax-check-only \"%s\" \"%s\" 2> %s"
outputFile = "/tmp/catalog-tmp.txt"
output = ''

success = 0
fail = 0
total = len(matches)

for orc in matches:
  ex = csoundCommand % (orc, orc[:-3] + 'sco', outputFile)
  retVal = os.system(ex)
  print orc, '\t', retVal
  if retVal != 0:
    output += "=======================\n"
    output += "%s\t%s\n"%(orc, retVal) 
    output += "=======================\n"
    f = open(outputFile,'r')
    for line in f:
      output += line 
    f.close()
    output += '\n\n'
    fail += 1
  else:
    success += 1

f = open('catalog_results.txt', 'w')
f.write(output)
f.flush()
f.close()

print "\nTESTS COMPLETE:\n\tSUCCESS\t%d\n\tFAIL\t%d\n\tTOTAL\t%d"%(success, fail, total)
