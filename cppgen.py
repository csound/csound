import re

f = open("H/csound.h","r")

contents = ""

for line in f:
    contents += line

print contents

#need to get to work with nested parens
a = re.findall(".*\(CSOUND .*\)", contents)

for i in a:
    print ">",i
