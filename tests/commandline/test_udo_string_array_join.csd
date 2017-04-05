<CsoundSynthesizer>
<CsInstruments>
ksmps = 32

opcode str_join, S, SS[] 
S_separator, S_vals[] xin

Sout init ""
isize = lenarray(S_vals)
icnt = 0

until (icnt >= isize) do
  Sout strcat Sout, S_separator
  Sout strcat Sout, S_vals[icnt]
  icnt += 1
od

xout Sout
endop

instr 1
Svals[] init 3
Svals[0] = "Hello"
Svals[1] = "World"
Svals[2] = "!"
Smessage = str_join(" ", Svals)
prints Smessage
turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
