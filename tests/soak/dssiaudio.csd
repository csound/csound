<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o DSSIplay_mono.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

gihandle dssiinit "caps.so", 19, 1	; = mono phaser and
gaout    init  0			; verbose about all ports

instr 1	; activate DSSI

dssiactivate gihandle, 1
endin

instr 2
ain1	diskin2 "beats.wav", 1,0,1	; loop

gaout = gaout+(ain1*.5)
endin

instr 3

dssictls gihandle, 0, .8, 1		; range -1 to 1
dssictls gihandle, 1, .05, 1		; rate 0 to 10
dssictls gihandle, 2, .8, 1		; depth 0 to 1
dssictls gihandle, 3, 2, 1		; spread 0 to 3.14
dssictls gihandle, 4, .7, 1		; feedback 0 to 0.999

endin

instr 4

aout1 dssiaudio gihandle, gaout	;get beats.wav, mono out
      outs aout1,aout1

gaout = 0

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
