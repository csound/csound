<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

instr 1		; Generate a random number from -1 to 1.
  
kbin =	birnd(1)
	printk .2, kbin

endin

</CsInstruments>
<CsScore>

i 1 0 1
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .

e

</CsScore>
</CsoundSynthesizer>
