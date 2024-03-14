<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o GEN43.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; Audacity shows the selection of reverb and noise- see image above

instr 1 ; analyze this selection and output result to pvx file
ires system_i 1,{{     
    pvanal -b1.04 -d0.8 finneganswake1.flac reverbnoise.pvx
        }}
endin

instr 2 ; untreated signal, contains some reverb and noise
asig    diskin2   "finneganswake1.flac", 1
outs    asig, asig
endin

instr 3 ; use .pvx file from instr. 1 to remove reverb and noise
; pvanal created 1 frame of size 1024, so size of table for GEN 43 = 512 (fftsize/2)
ipvx    ftgen 1, 0, 512, -43, "reverbnoise.pvx", 0    ; can be found in examples folder
asig    diskin2   "finneganswake1.flac", 1  
fsig    pvsanal   asig, 1024, 256, 1024, 1
fclean  pvstencil fsig, 0, 1, ipvx ; maximum cleaning
aclean  pvsynth   fclean
outs    aclean, aclean
endin

</CsInstruments>
<CsScore>
i1 0   2       ; first analyze
i2 5  12.7     ; untreated signal
i3 20 12.7     ; denoised gignal
e
</CsScore>
</CsoundSynthesizer>