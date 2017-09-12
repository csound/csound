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


instr 1	;Play an audio file

aleft, aright diskin2 "kickroll.wav", 1, 0
	      outs aleft, aright

endin


instr 2	;Cut-up stereo audio file.

ibps = 16
isubdiv = 2
ibarlength = 2
iphrasebars = 1
inumrepeats = 8

aleft, aright diskin2 "kickroll.wav", 1, 0
aleft, aright bbcuts aleft, aright, ibps, isubdiv, ibarlength, iphrasebars, inumrepeats
	      outs aleft, aright

endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 3 2
e

</CsScore>
</CsoundSynthesizer>
