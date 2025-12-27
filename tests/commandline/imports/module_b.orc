/* module_b.orc: Test module with same variable names but different values */

giSharedName = 200
sharedValue@global:i = 250

opcode ModuleBTest():(i,i)
    iout1 = giSharedName
    iout2 = sharedValue
    xout iout1, iout2
endop
