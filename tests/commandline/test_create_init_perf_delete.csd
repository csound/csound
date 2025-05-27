<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr Oscil
    out oscili(p4,p5)
endin

instr 10
   myInstance:Instr = create(Oscil)
   err1:i = init(myInstance)
   slid:k = expon(100, p3, 300)
   env:k = linen(0.1,0.1,p3,0.1)
   setp(myInstance, 5, slid)
   err2:k = perf(myInstance, env)
  // run at deinit time
   delete(myInstance) 
endin

;schedule(10,0,1)
</CsInstruments>
<CsScore>
f0 3
i10 0 2
</CsScore>
</CsoundSynthesizer>
