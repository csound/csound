<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dssictls.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

gihandle dssiinit "caps.so", 1, 1	; = equaliser and
gaoutl init  0				; verbose about all ports
gaoutr init  0

instr 1	; activate DSSI

dssiactivate gihandle, 1
endin

instr 2
ain1	diskin2 "beats.wav", 1,0,1	; loop

gaoutl = gaoutl+(ain1*.1)		; temper input
gaoutr = gaoutr+(ain1*.1)
endin

instr 3

dssictls gihandle, 2, -48, 1	; 31 Hz range -48 to 24
dssictls gihandle, 3, -48, 1	; 63 Hz range -48 to 24
dssictls gihandle, 4, -48, 1	; 125 Hz range -48 to 24
dssictls gihandle, 5, 20, 1	; 250 Hz range -48 to 24
dssictls gihandle, 6, -48, 1	; 500 Hz range -48 to 24
dssictls gihandle, 7, -48, 1	; 1 kHz Hz range -48 to 24
dssictls gihandle, 8, -48, 1	; 2 kHz range -48 to 24
dssictls gihandle, 9, 24, 1	; 4 kHz range -48 to 24
dssictls gihandle, 10, 24, 1	; 8 kHz range -48 to 24
dssictls gihandle, 11, 24, 1	; 16 kHz range -48 to 24

endin

instr 4

aout1, aout2	dssiaudio gihandle, gaoutl, gaoutr	;get beats.wav, mono out
		outs aout1,aout2

gaoutl = 0
gaoutr = 0
endin

</CsInstruments>
<CsScore>
i 1 0 20 
i 2 1 20
i 3 1 20
i 4 0 20

e
</CsScore>
</CsoundSynthesizer>
