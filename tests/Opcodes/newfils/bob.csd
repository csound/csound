<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o bob.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ga1 init 0
ga2 init 0

    seed 777

instr 7
a0  vco2     1, cpsmidinn(p4) ; a saw wave to be filtered
k1  jspline  0.7, 0.7, 1/0.7; add some jitter to filter resonance
kA  linseg   0,        p3/17, 1,     (17-2)*p3/17, 1,   p3/17, 0; overal amplitude envelope
k7  linseg   70,       p3/3,  700,   p3/3,     700,     p3/3,  70; filter cutoff envelope
k77 expseg   sqrt(7),  p3/2,  7.7,   p3/2,     sqrt(7); filter resonance modulation
a1  bob      a0 * kA, k7 - k1 * 70, k77 + k1, 3; PD bob~ ported
a1           /= 7; some normalization to avoid overload
aL           = p5 * a1; and then panning
aR           = a1 * (1 - p5)
    outs     aL, aR

ga1          +=  aL/7; AUX send
ga2          +=  aR/7
endin

instr 77 ; What a sound without some good reverb??
a1,a2  reverbsc ga1,ga2, 0.97+.1/7, 7777
    outs     a1, a2
ga1          = 0
ga2          = 0
endin
</CsInstruments>
<CsScore>
i 7  0 30 70 0.7; Perfect 7th
i 7  0 30 77 0.7
i 77 0 40
; In text editor try Find and Replace to substitute all 7 numbers for 3...8 )
; all 8s is a bit too high, all 3s is a bit too low, 4 sounds great!
; Do such replacement in initial 7 file, otherwise avoid accidental changes in the header...

; Tired of numerology? Try this:
; i 7  0 30 60 0.6
; i 7  0 32 45 0.4

e
</CsScore>
</CsoundSynthesizer>
