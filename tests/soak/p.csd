<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o p.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2022

instr 1

ipr	= p(4)
prints  "\nrandom number is now %f\n", ipr               ; find out the values of p4
aenv    linen   1, 0, p3, .2
asig    poscil  .5, p4
outs    asig * aenv, asig * aenv

endin

</CsInstruments>
<CsScore>
          
i1  0   1   [~ * 200]    ; random between 0-200    
i1  +   1   [~ * 200]
i1  +   2   [~ * 200]  

e
</CsScore>
</CsoundSynthesizer>
