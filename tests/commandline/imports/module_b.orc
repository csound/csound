/* module_b.orc: Test module with same variable names but different values */

giSharedName = 200
sharedValue@global:i = 250

opcode ModuleBTest, ii, 0
    xout giSharedName, sharedValue
endop
