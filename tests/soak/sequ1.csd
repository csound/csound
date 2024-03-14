<CsoundSynthesizer>

<CsInstruments>

ksmps = 32
0dbfs = 1.0
nchnls = 2

instr 1
;; rhythm array - these steps values multiplied by tempo (ticks) in BPM
irhythm[] fillarray 1, 1.5, 0.5, 0.5, 0.5, 0.5, 1.5, 1

;; instrument array - instrument number to render for each step
inst0[] fillarray 11, 12, 13, 14, 15, 16, 17, 18
inst1[] fillarray 19, 20, 21, 22, 23, 24, 25, 26

;; note array - here in cpsmidinn - esentially the 'p4' output from opcode
;;               can be any sequence of values
inotes[] fillarray 60, 61, 62, 63, 64, 65, 66, 67

;; variable tempo
kspeed line 60, p3, 180

;; rhythm, inst, notes, bpm, length, mode, verbose
kSeq0 sequ irhythm, inst0, inotes, kspeed, 8
kSeq1 sequ irhythm, inst1, inotes, kspeed * 1.2, 8
endin

instr 11, 12, 13, 14, 15, 16, 17, 18
kl linseg 0, p3*0.01, 1, p3*.99, 0
a1 oscil 0.9, cpsmidinn(p4)
outs1 a1*kl
endin

instr 19, 20, 21, 22, 23, 24, 25, 26
kl linseg 0, p3*0.01, 1,p3*.99, 0
a1 oscil 0.9, cpsmidinn(p4)
outs2 a1*kl
endin
</CsInstruments>

<CsScore>
i1 0 60
e
</CsScore>
</CsoundSynthesizer>
