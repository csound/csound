<CsTest>
description = "formant preservation for mincer, temposcal, and filescal"

[expect]
exit = 0
output = ["mincer formants ok", "temposcal formants ok", "filescal formants ok", "formant k-rate ok"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

opcode check_equal, 0, aakk
a1, a2, ktol, kmin xin
kdiff rms (a1 - a2)
klevel rms a1
if timeinsts() > 0.2 && !(kdiff <= ktol && klevel >= kmin && klevel < 1e30) then
  printks "equal check failed %.9f %.9f\n", 1, kdiff, klevel
  exitnowk(-1)
endif
endop

opcode check_differs, 0, aakk
a1, a2, ktol, kmin xin
kdiff rms (a1 - a2)
if timeinsts() > 0.2 && !(kdiff >= kmin && kdiff < 1e30) then
  printks "difference check failed %.9f\n", 1, kdiff
  exitnowk(-1)
endif
endop

opcode check_nonzero, 0, ak
a, kmin xin
klevel rms a
if timeinsts() > 0.2 && !(klevel >= kmin && klevel < 1e30) then
  printks "level check failed %.9f\n", 1, klevel
  exitnowk(-1)
endif
endop

instr 1
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
apos = 0.1
aoff mincer apos, 1, 2, itab, 2, 512, 4
adef mincer apos, 1, 2, itab, 2, 512, 4, 1, 0, 80
am1 mincer apos, 1, 2, itab, 2, 512, 4, 1, 1, 80
am2 mincer apos, 1, 2, itab, 2, 512, 4, 1, 2, 80
am3 mincer apos, 1, 2, itab, 2, 512, 4, 1, 3, 80
acoef mincer apos, 1, 2, itab, 2, 512, 4, 1, 1, 0
adeadoff mincer apos, 1, 1, itab, 2, 512, 4
adead mincer apos, 1, 1, itab, 2, 512, 4, 1, 2, 80
check_equal aoff, adef, 1e-7, 1e-6
check_differs am1, aoff, 1e-5, 1e-5
check_differs am2, aoff, 1e-5, 1e-5
check_equal am2, am3, 1e-7, 1e-6
check_equal am1, acoef, 1e-7, 1e-6
check_equal adeadoff, adead, 1e-7, 1e-6
check_nonzero am2, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "mincer formants ok\n", 0
  kdone = 1
endif
endin

instr 2
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
aoff temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1
adef temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1, 0, 80
am1 temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1, 1, 80
am2 temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1, 2, 80
am3 temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1, 3, 80
acoef temposcal 0.5, 1, 2, itab, 2, 512, 4, 1, 0, 1, 1, 1, 0
adeadoff temposcal 0.5, 1, 1, itab, 2, 512, 4, 1, 0, 1, 1
adead temposcal 0.5, 1, 1, itab, 2, 512, 4, 1, 0, 1, 1, 2, 80
check_equal aoff, adef, 1e-7, 1e-6
check_differs am1, aoff, 1e-5, 1e-5
check_differs am2, aoff, 1e-5, 1e-5
check_equal am2, am3, 1e-7, 1e-6
check_equal am1, acoef, 1e-7, 1e-6
check_equal adeadoff, adead, 1e-7, 1e-6
check_nonzero am2, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "temposcal formants ok\n", 0
  kdone = 1
endif
endin

instr 3
aoff filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1
adef filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 0, 80
am1 filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 1, 80
am2 filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 2, 80
am3 filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 3, 80
acoef filescal 0.5, 1, 2, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 1, 0
adeadoff filescal 0.5, 1, 1, "fox.wav", 2, 512, 4, 1, 0, 1, 1
adead filescal 0.5, 1, 1, "fox.wav", 2, 512, 4, 1, 0, 1, 1, 2, 80
check_equal aoff, adef, 1e-7, 1e-6
check_differs am1, aoff, 1e-5, 1e-5
check_differs am2, aoff, 1e-5, 1e-5
check_equal am2, am3, 1e-7, 1e-6
check_equal am1, acoef, 1e-7, 1e-6
check_equal adeadoff, adead, 1e-7, 1e-6
check_nonzero am2, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "filescal formants ok\n", 0
  kdone = 1
endif
endin

instr 4
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
apos = 0.1
aoff mincer apos, 1, 2, itab, 2, 512, 4
kkeep init 0
kcoefs init 8
if timeinsts() > 0.1 then
  kkeep = 1
  kcoefs = 80
endif
ak mincer apos, 1, 2, itab, 2, 512, 4, 1, kkeep, kcoefs
check_differs ak, aoff, 1e-5, 1e-5
check_nonzero ak, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "formant k-rate ok\n", 0
  kdone = 1
endif
endin

</CsInstruments>
<CsScore>
i1 0 0.4
i2 0 0.4
i3 0 0.4
i4 0 0.4
</CsScore>
</CsoundSynthesizer>
