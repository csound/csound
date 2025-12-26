<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* This is a baseline test - no imports, just a local UDO */
opcode MyOsc(kfreq):a
    aout = oscili(0.5, kfreq, -1, 0)
    xout aout
endop

instr 1
    aout = MyOsc(440)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
