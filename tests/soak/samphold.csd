<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o samphold.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kx	line -1, p3, 1   	; between -1 and +1 
ktrig	metro 1	 		; triggers 1 time per second 
kval	samphold kx, ktrig	; change value whenever ktrig = 1 
        printk2 kval		; will print every time kval changes 
asig	diskin2	"flute.aiff", 1+kval, 0, 1
	outs asig, asig

endin 
</CsInstruments> 
<CsScore> 

i 1 0 11 
e
</CsScore> 
</CsoundSynthesizer> 
