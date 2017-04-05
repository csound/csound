<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dssiinit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

gihandle dssiinit "g2reverb.so", 0, 1
gaout    init  0

instr 1	; activate DSSI

dssiactivate gihandle, 1
endin

instr 2

ain1	diskin2 "beats.wav", 1

gaout = gaout+(ain1*.3)
endin

instr 3 
	
dssictls gihandle, 4, 100, 1	; room 10 to 150 
dssictls gihandle, 5, 10, 1	; reverb time 1 to 20
dssictls gihandle, 6, .5, 1	; input bandwith 0 to 1
dssictls gihandle, 7, .25, 1	; damping 0 to 1
dssictls gihandle, 8, 0, 1	; dry -80 to 0
dssictls gihandle, 9, -10, 1	; reflections -80 to 0
dssictls gihandle, 10, -15, 1	; rev. tail -80 to 0
endin

instr 4

aout1, aout2 dssiaudio gihandle, gaout, gaout	;get beats.wav and
	     outs aout1,aout2	 		; stereo DSSI plugin

gaout = 0
endin
</CsInstruments>
<CsScore>
i 1 0 2 
i 2 1 10
i 3 1 10
i 4 0 10
e
</CsScore>
</CsoundSynthesizer>
