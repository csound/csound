<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pvsinit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Victor Lazzarini

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gfsig pvsinit 1024,256,1024,1

instr 1

a1  diskin p4
fs1 pvsanal a1,1024,256,1024,1
gfsig pvsmix fs1,gfsig
endin

instr 2
a1  pvsynth gfsig
outs a1, a1
; clear fsig
gfsig pvsgain gfsig,0
endin

</CsInstruments>
<CsScore>
i1 0 4 "fox.wav"
i1 0 4  "drumsMlp.wav"
i2 0 4
</CsScore>
</CsoundSynthesizer>
