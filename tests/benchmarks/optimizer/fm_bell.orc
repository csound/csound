instr 1
 kenv expseg 1,0.1,0.5,10,0.001
 amod oscili p4*2*kenv,p4*1.414
 acar oscili 0.1*kenv,p4+amod
 out acar
endin
