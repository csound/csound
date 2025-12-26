/* module_b.orc: Test module with same variable names but different values */

giSharedName = 200
sharedValue@global:i = 250

opcode ModuleBTest():(i,i)
    xout giSharedName, sharedValue
endop
