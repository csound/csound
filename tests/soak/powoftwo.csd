<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o poweroftwo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; bit reduction for Lo-Fi sound

iBits	  = p4						;bit depth
iQuantize = powoftwo(iBits)				;find number of discrete steps for this bit depth
iQuantize = iQuantize*0.5				;half the number of steps for each side of a bipolar signal
print	  iQuantize
asig	  soundin "fox.wav"
asig	  = round(asig * iQuantize) / iQuantize		;quantize audio signal (bit reduce)
	  outs asig, asig         

endin
</CsInstruments>
<CsScore>
;        bits
i1 0   3  16
i1 ^+3 .  12
i1 ^+3 .   8
i1 ^+3 .   4
i1 ^+3 .   2
i1 ^+3 .   1

e
</CsScore>
</CsoundSynthesizer>

