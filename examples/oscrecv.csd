<CsoundSynthesizer>

<CsInstruments>
instr 1
    OSCinit     7070
endin

instr 2
     kf1 init 0         
     kf2 init 0       
     kk  OSClisten "/foo/bar", "ff", kf1, kf2
     if kk =0 goto ex
        printk2 kf1
        printk2 kf2
ex:

endin

</CsInstruments>

<CsScore>
i1 0 1
i2 0 10
e
</CsScore>

</CsoundSynthesizer>
