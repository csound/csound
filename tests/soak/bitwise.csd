<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1
iresultOr = p4 | p5
iresultAnd = p4 & p5
prints "%i | %i  = %i\\n", p4, p5, iresultOr
prints "%i & %i  = %i\\n", p4, p5, iresultAnd
endin


instr 2 ; decimal to binary converter
Sbinary = ""
inumbits = 8
icount init inumbits - 1

pass:

	ivalue = 2 ^ icount
	if (p4 & ivalue >= ivalue) then
		Sdigit = "1"
	else
		Sdigit = "0"
	endif
	Sbinary strcat Sbinary, Sdigit

loop_ge icount, 1, 0, pass

Stext sprintf "%i is %s in binary\\n", p4, Sbinary
prints Stext
endin

</CsInstruments>
<CsScore>
i 1 0 0.1  1  2
i 1 +  .   1  3
i 1 +  .   2  4
i 1 +  .   3  10

i 2 2 0.1   12
i 2 +  .    9
i 2 +  .    15
i 2 +  .    49

e
</CsScore>
</CsoundSynthesizer>
