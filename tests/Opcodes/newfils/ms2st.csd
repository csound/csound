<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ms2st.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by  Menno Knevel - 2021
; sample = ambient sound of waterfall + animals (monkey, birds)

instr 1 ; MS sample is interleaved

am, as    diskin2    "MSjungle_interleaved.wav", 1, 0, 1 ; interleaved stereo, MS encoded
kwidth = p4                                              ; left = M, right = S
al, ar ms2st  am, as, kwidth
outs al, ar
endin

instr 2 ; 2 separate samples 

am    diskin2    "MSjungleMid.wav", 1        ; M sound as a separate mono sound
as    diskin2    "MSjungleSide.wav", 1       ; S sound as a separate mono sound
kwidth = p4     
al, ar ms2st  am, as, kwidth
outs al, ar

endin
</CsInstruments>
<CsScore>

i1 0 6.7 0          ; M only
i1 + 6.7 .3         ; M and a bit of S
i1 + 6.7 .5         ; M + S equal volume
i1 + 6.7 1          ; S only 

i2 28 6.7 0         ; M only
i2 +  6.7 .3        ; M and a bit of S
i2 +  6.7 .5        ; M + S equal volume
i2 +  6.7 1         ; S only
e
</CsScore>
</CsoundSynthesizer>
