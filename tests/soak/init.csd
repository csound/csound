<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-n  ;no sound output
</CsOptions>
<CsInstruments> 

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

instr 1 ;shows what init does
        kinit init 0 
        kinit = kinit + 1 
        printk .1, kinit 
endin 

instr 2 ;shows what an assignment does
        knoinit = 0 
        knoinit = knoinit + 1 
        printk .1, knoinit 
endin 
</CsInstruments> 
<CsScore> 
;play one second each
i1 0 1 
i2 2 1 
e
</CsScore> 
</CsoundSynthesizer> 
