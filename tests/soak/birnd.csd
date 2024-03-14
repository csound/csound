<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o birnd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1		; Generate a random number from -1 to 1.
  
kbin =	birnd(1)
	printk .2, kbin
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .
e
</CsScore>
</CsoundSynthesizer>
