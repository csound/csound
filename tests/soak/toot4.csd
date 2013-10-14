<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

          instr 4

iamp      =         ampdb(p4)           ; convert decibels to linear amp
iscale    =         iamp * .333         ; scale the amp at initialization
inote     =         cpspch(p5)          ; convert octave.pitch to cps


k1        linen     iscale, p6, p3, p7  ; p4=amp


a3        oscil     k1, inote*.996, 1   ; p5=freq
a2        oscil     k1, inote*1.004, 1  ; p6=attack time
a1        oscil     k1, inote, 1        ; p7=release time


a1        =         a1+a2+a3

          out       a1

          endin

</CsInstruments>
<CsScore>

f1   0    4096 10 1      ; sine wave

;ins strt dur  amp  freq      attack    release

i4   0    1    75   8.04      0.1       0.7

i4   1    1    70   8.02      0.07      0.6

i4   2    1    75   8.00      0.05      0.5

i4   3    1    70   8.02      0.05      0.4

i4   4    1    85   8.04      0.1       0.5

i4   5    1    80   8.04      0.05      0.5

i4   6    2    90   8.04      0.03      1.

</CsScore>
</CsoundSynthesizer>
