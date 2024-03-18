<CsoundSynthesizer>
<CsOptions>
-W -o lfsr.wav
</CsOptions>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1.0

; triangle wave
gitabsz init 2^13
giTri ftgen 1, 0, gitabsz, 7, 0, gitabsz/4, 1, gitabsz/2, -1, gitabsz/4, 0

; Grady Centaur scale
giCent = ftgen(100, 0, 128, -51,
               12, 2, 297.989, 60,
               1.0, 21/20, 9/8, 7/6, 5/4, 4/3, 7/5, 3/2, 14/9, 5/3, 7/4, 15/8, 2.0)

; just print the values
instr 1
    kn = lfsr(5, 128)
    printk2(kn)
endin

; play a melodic sequence
instr 2
    idur = p3
    iamp = p4
    iwav = p5
    itun = p6

    ; keep range small and transpose up 2 octaves to ensure audibility
    ; we're interpreting these number like MIDI notes numbers for easy table lookups
    kidx = lfsr(5, 128) + 24

    ktrig = metro(1)
    schedkwhen(ktrig, 0, 1, 3, 0, 1, iamp, iwav, itun, kidx)
endin

instr 3
    idur = p3
    iamp = ampdb(p4)
    iwav = p5
    itun = p6
    inote = p7

	kenv = linen(iamp, 0.1, idur, 0.1)
	aout = poscil3(kenv, tablekt(inote, itun), iwav)
	outs(aout, aout)
endin

</CsInstruments>
<CsScore>
i 1 0 0.25
i 2 0 10 -6 1 100

e
</CsScore>
</CsoundSynthesizer>