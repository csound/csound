<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o bbcuts.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2022

instr 1	;Play an audio file

aleft, aright diskin2 "drumsSlp.wav", 1, 0
	      outs aleft, aright
endin

instr 2	;Cut-up stereo audio file.

ibps = 16
isubdiv = p4
ibarlength = 2
iphrasebars = 1
inumrepeats = 8

aleft, aright diskin2 "drumsSlp.wav", 1, 0
aleft, aright bbcuts aleft, aright, ibps, isubdiv, ibarlength, iphrasebars, inumrepeats
	      outs aleft, aright
endin

</CsInstruments>
<CsScore>

i1 0  4		; original sample
i2 5  4 1	; subdivisions = 1
i2 10 4 .5	; subdivisions = .5
e

</CsScore>
</CsoundSynthesizer>
