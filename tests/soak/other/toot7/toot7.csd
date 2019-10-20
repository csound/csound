<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

          instr 7

ifunc1    =         p11                                ; initial waveform
ifunc2    =         p12                                ; crossfade waveform

ifad1     =         p3 * p13                           ; calculate initial fade (% of dur)
ifad2     =         p3 - ifad1                         ; calculate remaining duration

irel      =         .01                                ; set vibrato release time
idel1     =         p3 * p10                           ; calculate initial delay (% of dur)
isus      =         p3 - (idel1 + irel)                ; calculate remaining duration

iamp      =         ampdb(p4)
iscale    =         iamp * .166                        ; p4=amp
inote     =         cpspch(p5)                         ; p5=freq

k3        linseg    0, idel1, p9, isus, p9, irel, 0    ; p6=attack time
k2        oscil     k3, p8, 1                          ; p7=release time
k1        linen     iscale, p6, p3, p7                 ; p8=vib rate
a6        oscil     k1, inote*.998+k2, ifunc2          ; p9=vib depth
a5        oscil     k1, inote*1.002+k2, ifunc2         ; p10=vib delay (0-1)
a4        oscil     k1, inote+k2, ifunc2               ; p11=initial wave
a3        oscil     k1, inote*.997+k2, ifunc1          ; p12=cross wave
a2        oscil     k1, inote*1.003+k2, ifunc1         ; p13=fade time
a1        oscil     k1, inote+k2, ifunc1

kfade     linseg    1, ifad1, 0, ifad2, 1
afunc1    =         kfade * (a1+a2+a3)
afunc2    =         (1 - kfade) * (a4+a5+a6)

          out       afunc1 + afunc2

          endin

</CsInstruments>
<CsScore>

f1   0    2048 10   1                                                                ; Sine
f2   0    2048 10   1    0.5  0.3  0.25 0.2  0.167     0.14      0.125     .111      ; Sawtooth
f3   0    2048 10   1    0    0.3  0    0.2  0         0.14      0         .111      ; Square
f4   0    2048 10   1    1    1    1    0.7  0.5       0.3       0.1                 ; Pulse


;in st    dur  amp  frq       atk  rel  vbrt vbdp vbdl stw  ndw  crstm
i7   0    5    96   8.07      .03  .1   5    6    .99  1    2    .1
i7   6    5    96   8.09      .03  .1   5    6    .99  1    3    .1
i7  12    8    96   8.07      .03  .1   5    6    .99  1    4    .

e
</CsScore>
</CsoundSynthesizer>
