<CsoundSynthesizer>
<CsInstruments>

opcode print_str_array(vals:S[]):void
  icounter = 0 
  Sres = ""
  while (icounter < lenarray(vals)) do
    Sres = strcat(Sres, vals[icounter])
    icounter += 1
  od

  prints "print_str_array: "
  prints Sres
  prints "\n"
endop

instr 1
;create
SArr[] = [ "Test", " test2", " test 3"] 
ivals[] = [10, 20, 30] 

;print
icounter = 0
Sres = ""
while (icounter < 3) do
  print(ivals[icounter])
  Sres = strcat(Sres, SArr[icounter])
  icounter += 1
od
puts Sres, 1

print_str_array(SArr)

print_str_array(["I", " am", " an", " inline", " string", " array."])

endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
