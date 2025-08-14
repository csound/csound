<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
 cs:Csound = create()
 err:i = setoption(cs, "-n")
 err = compilestr(cs, {{
    0dbfs = 1
    instr 1
     a1 oscili p4, p5*(1+chnget:k("pitch"))
     out a1
     chnset(rms(a1),"rms")
    endin
    schedule(1,0,10,0.5,440);
}})
 err = start(cs)
 err2:k = perf(cs)
 freq:k line 0, p3, 1
 chnset(cs,freq,"pitch")
 amprms:k = chnget(cs, "rms")
 printk 0.1, amprms
 out inch(cs,1)
 delete(cs)
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>


