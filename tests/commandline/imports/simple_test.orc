/* simple_test.orc: Minimal test module with working UDO */

opcode TestOsc, a, k
    kfreq xin
    aout oscili 0.5, kfreq, -1, 0
    xout aout
endop

giTestValue = 42
myGlobalVar@global:i = 100
