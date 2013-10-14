<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o binit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1
;ain inch 1				; for live input
ain	diskin	 "beats.wav", 1		; input signal
fs1,fsi2 pvsifd	 ain, 2048, 512, 1		; ifd analysis
fst	partials fs1, fsi2, .003, 1, 3, 500	; partial tracking
fbins	binit	 fst, 2048		; convert it back to bins
aout	pvsynth	 fbins			; overlap-add resynthesis
	outs	 aout, aout

endin

</CsInstruments>
<CsScore>

i 1 0 2
e


</CsScore>
</CsoundSynthesizer>
