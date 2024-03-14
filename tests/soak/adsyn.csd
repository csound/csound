<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o adsyn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs =  1

; by Menno Knevel - 2021

ires system_i 1,{{ hetro -f100 -h100 stereoJungle.wav Jungle.het }} ; start at 100 Hz, harmonics up to 10kHz

instr 1         ; play original sample
aL,aR    diskin  "stereoJungle.wav", 1
        outs    aL, aR
endin

instr 2
kamod = 3       ; scale amplitude
kfmod = p4
ksmod = p5
asig	adsyn	kamod, kfmod, ksmod, "Jungle.het"
        outs	asig, asig
endin

</CsInstruments>
<CsScore>
s
i1 0 7                  ; original sample
s
;           frqmod speed
i2 0  20      1    .2   ; 5 x slower
i2 20 5       2     1   ; 2 x higher
i2 25 15     .3    1.5  ; lower & faster
e
</CsScore>
</CsoundSynthesizer>
