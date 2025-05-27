<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr Oscil
    kval = p5
    kamp = p4
    out oscili(kamp*0.1,kval)
endin

instr 10
   myInstance:Instr = create(Oscil)
   err1:i = init(myInstance,0.1,300)
   slid:k = expon(100, p3, 300)
   env:k = linen(1,0.1,p3,0.1)
   setp(myInstance, 5, slid)
   err2:k = perf(myInstance,env)
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
