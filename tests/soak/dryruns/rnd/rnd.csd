<CsoundSynthesizer>
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
