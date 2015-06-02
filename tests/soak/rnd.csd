<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o rnd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Andres Cabrera 2010

sr = 44100
ksmps = 4410
nchnls = 1
0dbfs = 1

instr 1
	; Generate a random number from 0 to 10.
	irand = rnd(10)
	print irand
endin

instr 2
	klimit init 10
	krand = rnd(klimit)
	printk 0, krand
endin

</CsInstruments>
<CsScore>

i 1 0 1  ; Generate 1 number
i 1 0 1  ; Generate another number
i 1 0 1  ; yet another number

i 2 2 1  ; 1 second prints 9 values (kr = 10)
e

</CsScore>
</CsoundSynthesizer>
