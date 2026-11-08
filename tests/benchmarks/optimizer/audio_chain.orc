instr 1
 a1 oscili 0.1,p4
 a2 = a1*0.9 + 0.001
 a3 = a2*0.8 + a1
 a4 = a3*0.7 + a2
 a5 = a4*0.6 + a3
 a6 = a5*0.5 + a4
 out a6*0.1
endin
