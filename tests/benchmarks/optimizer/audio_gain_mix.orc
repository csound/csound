; A simple synth voice with a control-rate gain and a second audio signal.
instr 1
 aInput vco2 0.08,p4
 aOther oscili 0.02,p4*1.005
 kGain linseg 0,0.02,1,0.2,0.3
 aResult = aInput*kGain+aOther
 out aResult
endin
