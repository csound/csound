<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kbeta line -1, p3, 1	;change beta value from -1 to 1
asig  noise .3, kbeta
asig  clip asig, 2, .9	;clip signal
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 10

e
</CsScore>
</CsoundSynthesizer>
