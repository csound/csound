<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Define a UDO locally without any imports */
opcode LocalOsc, a, k
    kfreq xin
    aout oscili 0.3, kfreq, -1, 0
    xout aout
endop

instr 1
    aout = LocalOsc(550)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
