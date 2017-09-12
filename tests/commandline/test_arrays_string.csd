<CsoundSynthesizer>
<CsInstruments>
instr 1
;create
SArr[] init 4

;fill
icounter = 0
until (icounter > 3) do
SArr[icounter] = "hello "
icounter += 1
od

;print
icounter = 0
Sres = ""
until (icounter > 3) do
Sres strcat Sres, SArr[icounter]
icounter += 1
od
puts Sres, 1
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
