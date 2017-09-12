<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

;test local aArrays
;jh march 2013

sr = 44100
ksmps = 32
nchnls = 2

instr 1
aArr[]     init       2
a1         oscils     .2, 400, 0
a2         oscils     .2, 500, 0
kEnv       transeg    1, p3, -3, 0
aArr[0]    =          a1 * kEnv * 20000
aArr[1]    =          a2 * kEnv * 20000
           outch      1, aArr[0], 2, aArr[1]
endin

instr 2 ;to test identical names
aArr[]     init       2
a1         oscils     .2, 600, 0
a2         oscils     .2, 700, 0
kEnv       transeg    0, p3-p3/10, 3, 1, p3/10, -6, 0
aArr[0]    =          a1 * kEnv * 20000
aArr[1]    =          a2 * kEnv * 20000
           outch      1, aArr[0], 2, aArr[1]
endin
</CsInstruments>
<CsScore>
i 1 0 3
i 2 0 3
</CsScore>
</CsoundSynthesizer>

