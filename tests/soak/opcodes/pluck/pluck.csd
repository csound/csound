<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps = 220
icps = 220
ifn  = 0
imeth = p4

asig pluck 0.7, 220, 220, ifn, imeth, .1, 10
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0  5 1
i 1 5  5 4	;needs 2 extra parameters (iparm1, iparm2)
i 1 10 5 6
e
</CsScore>
</CsoundSynthesizer>
