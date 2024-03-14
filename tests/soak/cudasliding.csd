<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cudaslidig.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
0dbfs = 1
nchnls = 2


instr 1
asig = diskin:a("flutec3.wav",1,0,1)
amod = oscil:a(1,3)
asig2 = cudasliding(asig,amod)
asig = linenr(asig2,0.005,0.01,0.01)    
   out(asig)
endin



</CsInstruments>
<CsScore>
i1 0 60

</CsScore>
</CsoundSynthesizer>

