<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpspch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; Convert pitch-class value into Hz 
  
ipch =	p4
icps =	cpspch(ipch)
	print icps
asig	oscil 0.7, icps, 1
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 1 8.01
i 1 + 1 8.02
i 1 + 1 8.03
i 1 + .5 5.09

e
</CsScore>
</CsoundSynthesizer>
