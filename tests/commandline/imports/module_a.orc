/* module_a.orc: Test module with both g-prefix and @global variables */

giSharedName = 100
sharedValue@global:i = 150

opcode ModuleATest():(i,i)
    iout1 = giSharedName
    iout2 = sharedValue
    xout iout1, iout2
endop
