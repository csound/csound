<CsoundSynthesizer>

<CsInstruments>
ksmps = 32
0dbfs = 1.0
nchnls = 2

instr 1
;; rhythm array - these values are multiplied by tempo (ticks) in BPM
irhythm0[] fillarray 1, 1.5, 0.5, 0.5, 0.5, 0.5, 1.5, 1
;; instrument array - instrument number to render for each step
iinsts0[] fillarray 11, 12, 13, 14, 15, 16, 17, 18
;; note array - here cpsmidinn(p4), amps(p5), mod ratios(p6), mod indices(p7)
;; - esentially the 'p4', 'p5', 'p6' and 'p7' are output from sequ
inotes[][] init 4,8 ;initialize 4 rows with 8 columns - p4=pitch, p5=amp, p6=modratio, p7=modindex
inotes fillarray 60, 61, 62, 63, 64, 65, 66, 67, \
                 0.8, 0.3, 0.6, 0.2, 0.7, 0.4, 0.5, 0.6, \
                 1, 2, 3, 4, 5, 6, 7, 8, \
                 1, 11, 2, 12, 3, 21, 4, 22
;; NOTE: this can be any sequence of values
;; variable tempo
kspeed linseg 85, p3*.7, 85, p3*.3, 240
;; rhythms, insts, notes, bpm, length, mode, step, reset, verbose
kSeq sequ irhythm0, iinsts0, inotes, kspeed, 8, p4
endin

instr 11, 12, 13, 14, 15, 16, 17, 18
kenv linseg 0, p3*0.01, 1, p3*.99, 0
asig foscil p5, cpsmidinn(p4), 1, p6, p7
outall asig * kenv
endin

</CsInstruments>

<CsScore>
i1 0 15 0 ;; forward mode
s
f0 1
s
i1 0 15 -1 ;; backward mode
s
f0 1
s
i1 0 15 -2 ;; forward and backward mode
s
f0 1
s
i1 0 15 -3 ;; random
s
f0 1
s
i1 0 6 -4 ;; play forward once and stop
s
f0 1
s
i1 0 6 -5 ;; play backward once and stop
s
f0 1
s
i1 0 15 -6 ;; shuffle mode
s
f0 1
s
i1 0 30 1 ;; mutate after each step
s
f0 1
s
i1 0 30 2 ;; mutate each second step
s
f0 1
s
i1 0 30 4 ;; mutate every four steps
e
</CsScore>
</CsoundSynthesizer>


