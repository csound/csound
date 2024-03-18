<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual   -M0  ;;;realtime audio out and midi in 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o midicontrolchange.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; use slider of contr. 7 of virtual keyboard
kcont init 1	; max. volume
midicontrolchange 7, kcont, 0, 1; use controller 7, scaled between 0 and 1	
printk2 kcont	; Display the key value when it changes and key is pressed

kenv madsr 0.5, 0.8, 0.8, 0.5		; envelope multiplied by
asig pluck kenv*kcont, 220, 220, 2, 1	; value of controller 7	 
     outs  asig, asig

endin
</CsInstruments>
<CsScore>
f 0 30
f 2 0 4096 10 1	

i 1 10 2	; play a note from score as well
e

</CsScore>
</CsoundSynthesizer>
