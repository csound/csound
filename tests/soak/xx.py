#!/usr/bin/python

import os
import sys

csound = "../../csound"
flags = "-d -W -m0"

testFiles = [
    cpumeter
]



if len(sys.argv) == 2:
    csound = sys.argv[1]
else:
    print "Testing git version"


print "Using Csound Command: " + csound    

try:
    os.remove("Old_OutputX")
except OSError:
    pass

try:
    os.remove("CheckSumsX")
    os.remove("OutputX")
except OSError:
    pass

try:
    os.rename("Output1", "Old_Output")
except OSError:
    pass

for filename in testFiles:

    replaceText = (csound, flags, filename, filename)
    csCommand = "%s %s %s.csd -o ./%s.wav >> OutputX 2>&1"%replaceText
  
    md5sumCommand = "md5sum -b %s.wav >> CheckSumsX"%filename

    print csCommand
    os.system(csCommand)
    os.system(md5sumCommand)

    try:
        os.remove(filename + ".wav")
    except OSError:
        print "Error: %s.wav was not generated"%filename
    
print "********Comparing checksums"
os.system("diff CheckSumsX SAFESumsX")
print "********Comparing output"
os.system("sed -e /Elapsed/d < OutputX > OutputX1")
os.system("diff OutputX1 Old_OutputX | grep -v Elapsed")
