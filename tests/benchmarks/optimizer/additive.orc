instr 1
 kenv linseg 0,0.01,1,0.1,0.7,10,0
 a1 oscili 0.1,p4
 a2 oscili 0.05,p4*2
 a3 oscili 0.025,p4*3
 a4 oscili 0.0125,p4*4
 out (a1+a2+a3+a4)*kenv
endin
