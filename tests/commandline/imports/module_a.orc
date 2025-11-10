/* module_a.orc: Test module with both g-prefix and @global variables */

giSharedName = 100
sharedValue@global:i = 150

opcode ModuleATest, ii, 0
    xout giSharedName, sharedValue
endop
