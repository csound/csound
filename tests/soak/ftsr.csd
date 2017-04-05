<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform 
-odac     ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too 
; For Non-realtime ouput leave only the line below: 
; -o ftsr.wav -W ;;; for file output any platform 
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1

itab = p4
isr = ftsr(itab)
prints "sampling-rate of table number %d = %d\n", itab, isr
    
endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "kickroll.wav" 0 0 0	;stereo file
f 2 0 0 1 "ahh.aiff" 0 0 0	;& different formats
f 3 0 0 1 "beats.mp3" 0 0 0
f 4 0 0 1 "beats.ogg" 0 0 0

i 1 0 1 1
i 1 + 1 2
i 1 + 1 3
i 1 + 1 4
e
</CsScore>
</CsoundSynthesizer>
