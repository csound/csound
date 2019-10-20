<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
          instr 5

irel      =         0.01                               ; set vibrato release time
idel1     =         p3 * p10                           ; calculate initial delay (% of dur)
isus      =         p3 - (idel1 + irel)                ; calculate remaining duration

iamp      =         ampdb(p4)
iscale    =         iamp * .333                        ; p4=amp
inote     =         cpspch(p5)                         ; p5=freq


k3        linseg    0, idel1, p9, isus, p9, irel, 0    ; p6=attack time
k2        oscil     k3, p8, 1                          ; p7=release time
k1        linen     iscale, p6, p3, p7                 ; p8=vib rate

a3        oscil     k1, inote*.995+k2, 1               ; p9=vib depth
a2        oscil     k1, inote*1.005+k2, 1              ; p10=vib delay (0-1)
a1        oscil     k1, inote+k2, 1

          out       a1+a2+a3

          endin

</CsInstruments>
<CsScore>

f1   0    4096 10 1      ; sine wave

;ins strt dur  amp  freq      atk  rel  vibrt     vbdpt     vbdel

i5   0    3    86   10.00     0.1  0.7  7         6         .4

i5   4    3    86   10.02     1    0.2  6         6         .4

i5   8    4    86   10.04     2    1    5         6         .4

</CsScore>
</CsoundSynthesizer>
