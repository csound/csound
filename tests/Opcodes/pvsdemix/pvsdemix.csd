 <CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pvsdemix.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     
ifftsize = 2048 
iwtype   = 1   
kpos  = p4

asig1 diskin2 "fox.wav",1,0,1
asig2 diskin2 "drumsMlp.wav",1,0,1
aL, aR pan2 asig1,0.25
aL1, aR1 pan2 asig2,0.75

if p5 == 0 then
    outs aL1+aL, aR1+aR
else
    flc    pvsanal   aL1+aL, ifftsize, ifftsize/4, ifftsize, iwtype
    frc    pvsanal   aR1+aR, ifftsize, ifftsize/4, ifftsize, iwtype
    fdm    pvsdemix  flc, frc,kpos,20,100
    adm    pvsynth   fdm
    adm     *= 0.5
    outs adm, adm
endif

endin

</CsInstruments>
<CsScore>

;         pan   choice
i 1 0 5    0      0     ; bypass
i 1 5 5  -0.5     1     ; fox
i 1 10 5  0.5     1     ; beats

e
</CsScore>
</CsoundSynthesizer>
