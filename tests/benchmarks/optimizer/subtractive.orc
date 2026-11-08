instr 1
 a1 vco2 0.1,p4
 kenv linseg 0,0.01,1,0.1,0.5,10,0
 afilter moogladder a1,300+kenv*4000,0.3
 out afilter*kenv
endin
