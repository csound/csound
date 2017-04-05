<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tab.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifn1 ftgen 1, 0, 0, 1, "flute.aiff", 0, 4, 0	;deferred-size table

instr 1 

atab  init 0 
isize tableng 1					;length of table?
print isize
andx  phasor 1 / (isize / sr) 
asig  tab andx, 1, 1				;has a 0 to 1 range
      outs asig, asig

endin 
</CsInstruments> 
<CsScore> 

i 1 0 2.3
e
</CsScore> 
</CsoundSynthesizer>
