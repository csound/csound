<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

struct ModRef val:i

instr 1
    stm:ModRef init 42
    print stm.val
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
