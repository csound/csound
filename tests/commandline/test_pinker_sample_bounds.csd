<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 2
 aOut pinker
 aGate = 1
 kCount init 0
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kActual vaget kN, aOut
  if kActive == 0 then
   if kActual != 0 then
    printks "pinker inactive sample=%g output=%g\n", 0, kN, kActual
    exitnowk(-1)
   endif
  else
   kCount += 1
  endif
  kN += 1
 od
 if kCount == p4 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 2 then
  prints "pinker checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Leading and trailing inactive samples, including a note inside one block.
i 2 .0006103515625 .0042724609375 35
i 2 .1256103515625 .0003662109375 3
i 99 .15 .001
e
</CsScore>
</CsoundSynthesizer>
