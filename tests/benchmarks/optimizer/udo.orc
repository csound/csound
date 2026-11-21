opcode Gain,a,ak
 setksmps 1
 asignal,kgain xin
 xout asignal*kgain
endop
instr 1
 a1 oscili 0.1,p4
 a2 Gain a1,0.9
 a3 Gain a2,0.8
 a4 Gain a3,0.7
 out a4
endin
