#!/usr/bin/python

# Csound6 Regresson Test Suite
# By Steven Yi<stevenyi at gmail dot com>John ffitch

import os
import sys

from testUI import TestApplication
from Tkinter import *

showUIatClose = False
csoundExecutable = ""

class Test:
    def __init__(self, fileName, description, expected=True):
        self.fileName = fileName
        self.description = ""
        self.expected = expected

def showUI(results):
    root = Tk()
    app = TestApplication(master=root)
    app.setResults(results)
    app.mainloop()
    root.destroy()

def showHelp():
    message = """Csound Regression Test Suite by John ffitch

    Runs tests and shows return values of tests. Results
    are written to results.txt file.
    """

    print message

def runTest():
    runArgs = "-Wd -n"

    tests = [
        ["buga1.csd", " "],
        ["buga.csd", " "],
        ["bugas.csd", " "],
        ["bugb.csd", " "],
        ["bugf.csd", " "],
        ["buginit.csd", " "],
        ["bugl.csd", " "],
        ["bugm.csd", " "],
        ["bugn.csd", "named instruments"],
        ["bugs.csd", "multipe strings in score"],
        ["bugsort.csd", "memory problem in sort"],
        ["bugst.csd", "string in score"],
        ["bugtrans.csd", "transegr"],
        ["bugbra.csd", "Open bracket in expression"],
        ["bugy.csd", "y in score"],
        ["bugbigargs.csd", "many arguments in score"],
        ["bugadsr.csd", "xadsr"],
        ["bug18.csd", "gen18"],
        ["bugaa.csd", "arate array arithmetic"],
        ["bugarray.csd", "krate array arithmetic"],
        ["bugausst.csd", "gausstrig"],
        ["bugblam.csd", "macro test"],
        ["bugcopy.csd", "copy mixed rate arrays"],
        ["bugftlen.csd", "table size oddity"],
        ["buglosc.csd", "non integer loscil"],
        ["bugpow.csd", "karray power"],
        ["bugrezzy.csd", "rezzy stabilised"],
        ["buggen31.csd", "gen31 test"],
        ["bugg.csd", "grain3"],
        ["bugline.csd", "comments in score"],
        ["arrayout.csd", "array dimension greater than nchls"],
        ["bugstr1.csd", "escaes in score strings"]
    ]

    output = ""
    tempfile = "/tmp/csound_test_output.txt"
    counter = 1

    retVals = []

    testPass = 0
    testFail = 0

    for t in tests:
        filename = t[0]
        desc = t[1]
        expectedResult = (len(t) == 3) and 1 or 0

        if(os.sep == '\\'):
            executable = (csoundExecutable == "") and "..\..\csound.exe" or csoundExecutable
            command = "%s %s %s 2> %s"%(executable, runArgs, filename, tempfile)
            print command
            retVal = os.system(command)
        else:
            executable = (csoundExecutable == "") and "../../csound" or csoundExecutable
            command = "%s %s %s 2> %s"%(executable, runArgs, filename, tempfile)
            print command
            retVal = os.system(command)
  
        out = ""
        if (retVal == 0) == (expectedResult == 0):
            testPass += 1
            out = "[pass] - "
        else:
            testFail += 1
            out = "[FAIL] - "

        out += "Test %i: %s (%s)\n\tReturn Code: %i\tExpected: %d\n"%(counter, desc, filename, retVal, expectedResult
)
        print out
        output += "%s\n"%("=" * 80)
        output += "Test %i: %s (%s)\nReturn Code: %i\n"%(counter, desc, filename, retVal)
        output += "%s\n\n"%("=" * 80)
        f = open(tempfile, "r")

        csOutput = ""

        for line in f:
            csOutput += line

        output += csOutput

        f.close()

        retVals.append(t + [retVal, csOutput])

        output += "\n\n"
        counter += 1

#    print output

    print "%s\n\n"%("=" * 80)
    print "Tests Passed: %i\nTests Failed: %i\n"%(testPass, testFail)


    f = open("results.txt", "w")
    f.write(output)
    f.flush()
    f.close()

    return retVals

if __name__ == "__main__":
    if(len(sys.argv) > 1):
        for arg in sys.argv:
            if (arg == "--help"):
                showHelp()
                sys.exit(0)
            elif arg == "--show-ui":
                showUIatClose = True
            elif arg.startswith("--csound-executable="):
                csoundExecutable = arg[20:]
                print csoundExecutable
            elif arg.startswith("--opcode6dir64="):
                os.environ['OPCODE6DIR64'] = arg[15:]
                print os.environ['OPCODE6DIR64'] 
    results = runTest()
    if (showUIatClose):
        showUI(results)
