<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

    instr 6

  ifunc     =         p11                                ; select the basic waveform
  irel      =         0.01                               ; set vibrato release time
  idel1     =         p3 * p10                           ; calculate initial delay (% of dur)
  isus      =         p3 - (idel1 + irel)                ; calculate remaining duration

  iamp      =         ampdb(p4)
  iscale    =         iamp * .333                        ; p4=amp
  inote     =         cpspch(p5)                         ; p5=freq

  k3        linseg    0, idel1, p9, isus, p9, irel, 0    ; p6=attack time
  k2        oscil     k3, p8, 1                          ; p7=release time
  k1        linen     iscale, p6, p3, p7                 ; p8=vib rate

  a3        oscil     k1, inote*.999+k2, ifunc           ; p9=vib depth
  a2        oscil     k1, inote*1.001+k2, ifunc          ; p10=vib delay (0-1)
  a1        oscil     k1, inote+k2, ifunc

  out       a1+a2+a3

    endin

</CsInstruments>
<CsScore>

f1   0    2048 10   1                                                                ; Sine

f2   0    2048 10   1    0.5  0.3  0.25 0.2  0.167     0.14      0.125     .111      ; Sawtooth

f3   0    2048 10   1    0    0.3  0    0.2  0         0.14      0         .111      ; Square

f4   0    2048 10   1    1    1    1    0.7  0.5       0.3       0.1                 ; Pulse



;ins strt dur  amp       frq       atk       rel       vbrt vbdpt     vibdl     waveform

i6   0     2   86        8.00      .03       .7        6    9         .8        1

i6   3     2   86        8.02      .03       .7        6    9         .8        2

i6   6     2   86        8.04      .03       .7        6    9         .8        3

i6   9     3   86        8.05      .03       .7        6    9         .8

</CsScore>
</CsoundSynthesizer>
