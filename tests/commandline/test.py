#!/usr/bin/python

# Csound Test Suite
# By Steven Yi <stevenyi at gmail dot com>

import os
import sys

from testUI import TestApplication
from Tkinter import *

parserType = ""
showUIatClose = False
##csoundExecutable = r"C:/Users/new/csound-csound6-git/csound.exe "
csoundExecutable =""

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
    message = """Csound Test Suite by Steven Yi<stevenyi@gmail.com>

    Runs tests using new parser and shows return values of tests. Results
    are written to results.txt file.  To show the results using a UI, pass
    in the command "--show-ui" like so:

    ./test.py --show-ui
    
    The test suite defaults to using the new parser.  To use the old parser for 
    the tests, use "--old-parser" in the command like so:
    
    ./test.py --show-ui --old-parser
    
    """

    print message

def runTest():
    runArgs = "-Wdo test.wav"

    if (parserType == "--old-parser"):
        print "Testing with old parser"
    else:
        print "Testing with new parser"

    tests = [
        ["test1.csd", "Simple Test, Single Channel"],
        ["test2.csd", "Simple Test, 2 Channel"],
        ["test3.csd", "Simple Test, using i-rate variables, 2 Channel"],
        ["test4.csd", "Simple Test, using k-rate variables, 2 Channel"],
        ["test5.csd", "Simple Test, using global i-rate variables, 2 Channel"],
        ["test6.csd", "Testing Pfields"],
        ["test7.csd", "Testing expressions, no functions"],
        ["test8.csd", "Testing multi-part expressions, no functions"],
        ["test9.csd", "Unused Label (to test labels get parsed)"],
        ["test10.csd", "kgoto going to a label"],
        ["test11.csd", "if-kgoto going to a label, boolean expressions"],
        ["test12.csd", "Simple if-then statement"],
        ["test13.csd", "function call"],
        ["test14.csd", "polymorphic test, 0xffff (init)"],
        ["test15.csd", "pluck test, 0xffff (init)"],
        ["test16.csd", "Simple if-then with multiple statements in body"],
        ["test17.csd", "Simple if-then-else with multiple statements in body"],
        ["test18.csd", "if-then-elseif with no else block"],
        ["test19.csd", "if-elseif-else"],
        ["test20.csd", "if-elseif-else with inner if-elseif-else blocks"],
        ["test21.csd", "if-elseif-else with multiple elseif blocks"],
        ["test22.csd", "simple UDO"],
        ["test23.csd", "named instrument"],
##        ["test24.csd", "la_i opcodes"],
        ["test43.csd", "mixed numbered and named instruments"],
        ["test25.csd", "polymorphic test, 0xfffd (peak)"],
        ["test26.csd", "polymorphic test, 0xfffc (divz)"],
        ["test27.csd", "polymorphic test, 0xfffb (chnget)"],
        ["test28.csd", "label test"],
        ["test29.csd", "bit operations test"],
        ["test30.csd", "multi-numbered instrument test"],
        ["test31.csd", "i-rate conditional test"],
        ["test32.csd", "continuation lines test"],
        ["test33.csd", "using named instrument from score (testing score strings)"],
        ["test34.csd", "tertiary conditional expressions"],
        ["test35.csd", "test of passign"],
        ["test36.csd", "opcode with all input args optional (passign)"],
        ["test37.csd", "Testing in and out"],
        ["test38.csd", "Testing simple macro"],
        ["test39.csd", "Testing macro with argument"],
        ["test40.csd", "Testing i^j"],
        ["test41.csd", "if statement with = instead of == gives a failure", 1],
        ["test42.csd", "extended string"],
	["test44.csd", "expected failure with in-arg given to in opcode", 1],
	["test45.csd", "if-goto with expression in boolean comparison"],
	["test46.csd", "if-then with expression in boolean comparison"],
	["test47.csd", "until loop and t-variables"],
	["test48.csd", "expected failure with variable used before defined", 1],
	["test_instr0_labels.csd", "test labels in instr0 space"],
	["test_string.csd", "test string assignment and printing"],
	["test_sprintf.csd", "test string assignment and printing"],
	["test_sprintf2.csd", "test string assignment and printing that causes reallocation"],
	["test_label_within_if_block.csd", "test label within if block"],

	["test_arrays.csd", "test k-array with single dimension, assignment to expression value"],
	["test_arrays2.csd", "test gk-array with single dimension, assignment to expression value"],
	["test_arrays3.csd", "test k-array with single dimension, assignment with number"],
	["test_arrays_multi.csd", "test multi-dimensionsl k-array, assigment to number and expression"],
	["test_arrays_string.csd", "test string-array"],
	["test_arrays_string2.csd", "test simple string-array assignment"],
	["test_asig_as_array.csd", "test using a-sig with array get/set syntax"],
	
	["test_empty_conditional_branches.csd", "tests that empty branches do not cause compiler issues"],
	["test_empty_instr.csd", "tests that empty instruments do not cause compiler issues"],
	["test_empty_udo.csd", "tests that empty UDOs do not cause compiler issues"],

	["test_semantics_undefined_var.csd", "test undefined var", 1],
	["test_invalid_expression.csd", "test expression", 1],
	["test_invalid_ternary.csd", "test expression", 1],

	["test_opcode_as_function.csd", "test expression"],
	["test_fsig_udo.csd", "UDO with f-sig arg"],
	["test_karrays_udo.csd", "UDO with k[] arg"],
	["test_arrays_addition.csd", "test array arithmetic (i.e. k[] + k[]"],
	["test_arrays_fns.csd", "test functions on arrays (i.e. tabgen)"],
	["test_polymorphic_udo.csd", "test polymorphic udo"],
	["test_udo_a_array.csd", "test udo with a-array"],
	["test_udo_2d_array.csd", "test udo with 2d-array"],
        ["test_udo_string_array_join.csd", "test udo with S[] arg returning S"],
        ["test_array_function_call.csd", "test synthesizing an array arg from a function-call"],
        ["test_explicit_types.csd", "test typed identifiers (i.e. signals:a[], sigLeft:a)"],
        ["test_parser3_opcall_ambiguities.csd", "test T_OPCALL ambiguities"],
        ["test_new_udo_syntax.csd", "test new-style UDO syntax"],
    ]

    arrayTests = [["arrays/arrays_i_local.csd", "local i[]"],
        ["arrays/arrays_i_global.csd", "global i[]"],
        ["arrays/arrays_k_local.csd", "local k[]"],
        ["arrays/arrays_k_global.csd", "global k[]"],
        ["arrays/arrays_a_local.csd", "local a[]"],
        ["arrays/arrays_a_global.csd", "global a[]"],
        ["arrays/arrays_S_local.csd", "local S[]"],
        ["arrays/arrays_S_global.csd", "global S[]"],
    ]


    structTests = [["structs/test_structs.csd", "basic struct test"],
    ]

    tests += arrayTests + structTests

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
            executable = (csoundExecutable == "") and "..\csound.exe" or csoundExecutable
            command = "%s %s %s %s 2> %s"%(executable, parserType, runArgs, filename, tempfile)
            print command
            retVal = os.system(command)
        else:
            executable = (csoundExecutable == "") and "../../csound" or csoundExecutable
            command = "%s %s %s %s &> %s"%(executable, parserType, runArgs, filename, tempfile)
            #print command
            retVal = os.system(command)
  
        out = ""
        if (retVal == 0) == (expectedResult == 0):
            testPass += 1
        else:
            testFail += 1
            out = "[FAIL] - Test %i: %s (%s)\n\tReturn Code: %i\tExpected: %d\n"%(counter, desc, filename, retVal, expectedResult)
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
            elif arg == "--old-parser":
                parserType = "--old-parser"
            elif arg.startswith("--csound-executable="):
                csoundExecutable = arg[20:]
                print csoundExecutable
            elif arg.startswith("--opcode6dir64="):
                os.environ['OPCODE6DIR64'] = arg[15:]
                print os.environ['OPCODE6DIR64'] 
    results = runTest()
    if (showUIatClose):
        showUI(results)
