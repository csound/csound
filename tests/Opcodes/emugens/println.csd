<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1

instr 1
  ; printsk is executed at k-time, possibly multiples times per cycle.
  k0 = 0
  while k0 < 20 do
    if k0 % 2 == 0 then
      printsk "k0 = %d\n", k0
    endif
    k0 += 1
  od
endin

instr 2
  ; println is similar to printsk but appends a new line
  k0 = 0
  while k0 < 20 do
    if k0 % 2 == 0 then
      println "k0 = %d", k0
    endif
    k0 += 1
  od
endin


</CsInstruments>
<CsScore>

i 1 0 0.1
i 2 0.1 0.1
e


</CsScore>
</CsoundSynthesizer>
