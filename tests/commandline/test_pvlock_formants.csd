<CsTest>
description = "formant preservation for mincer, temposcal, and filescal"

[expect]
exit = 0
output = ["mincer formants ok", "klock0 formants ok", "control rounding ok", "temposcal formants ok", "filescal formants ok", "formant k-rate ok", "raw pitch bypass ok", "sparse gain bounded ok", "formant state reset ok"]
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

opcode check_bounded, 0, akk
a, klimit, kmin xin
klevel rms a
if timeinsts() > 0.2 && !(klevel >= kmin && klevel < klimit) then
  printks "bounded check failed %.9f\n", 1, klevel
  exitnowk(-1)
endif
endop

opcode check_equal_window, 0, aakkkkS
a1, a2, ktol, kmin, kstart, kend, Slabel xin
kdiff rms (a1 - a2)
klevel rms a1
ktime = timeinsts()
if ktime > kstart && ktime < kend && !(kdiff <= ktol && klevel >= kmin && klevel < 1e30) then
  printks "%s equal window check failed %.9f %.9f\n", 1, Slabel, kdiff, klevel
  exitnowk(-1)
endif
endop

opcode check_differs_window, 0, aakkkkS
a1, a2, ktol, kmin, kstart, kend, Slabel xin
kdiff rms (a1 - a2)
ktime = timeinsts()
if ktime > kstart && ktime < kend && !(kdiff >= ktol && kdiff < 1e30) then
  printks "%s difference window check failed %.9f\n", 1, Slabel, kdiff
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
amhalf mincer apos, 1, 2, itab, 2, 512, 4, 1, 0.5, 80
acoef1 mincer apos, 1, 2, itab, 2, 512, 4, 1, 1, 1
acoef06 mincer apos, 1, 2, itab, 2, 512, 4, 1, 1, 0.6
adeadoff mincer apos, 1, 1, itab, 2, 512, 4
adead mincer apos, 1, 1, itab, 2, 512, 4, 1, 2, 80
check_equal aoff, adef, 1e-7, 1e-6
check_differs am1, aoff, 1e-5, 1e-5
check_differs am2, aoff, 1e-5, 1e-5
check_equal am2, am3, 1e-7, 1e-6
check_equal am1, acoef, 1e-7, 1e-6
check_equal amhalf, am1, 1e-7, 1e-6
check_equal acoef06, acoef1, 1e-7, 1e-6
check_differs acoef1, am1, 1e-5, 1e-5
check_equal adeadoff, adead, 1e-7, 1e-6
check_nonzero am2, 1e-6
aoffk0 mincer apos, 1, 2, itab, 0, 512, 4
am1k0 mincer apos, 1, 2, itab, 0, 512, 4, 1, 1, 80
am2k0 mincer apos, 1, 2, itab, 0, 512, 4, 1, 2, 80
check_differs am1k0, aoffk0, 1e-5, 1e-5
check_differs am2k0, aoffk0, 1e-5, 1e-5
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "mincer formants ok\n", 0
  printks "klock0 formants ok\n", 0
  printks "control rounding ok\n", 0
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
aoffk0 temposcal 0.5, 1, 2, itab, 0, 512, 4, 1, 0, 1, 1
am1k0 temposcal 0.5, 1, 2, itab, 0, 512, 4, 1, 0, 1, 1, 1, 80
check_differs am1k0, aoffk0, 1e-5, 1e-5
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
aoffk0 filescal 0.5, 1, 2, "fox.wav", 0, 512, 4, 1, 0, 1, 1
am1k0 filescal 0.5, 1, 2, "fox.wav", 0, 512, 4, 1, 0, 1, 1, 1, 80
check_differs am1k0, aoffk0, 1e-5, 1e-5
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
adef mincer apos, 1, 2, itab, 2, 512, 4, 1, 0, 8
kkeep init 0
kcoefs init 8
if timeinsts() > 0.1 then
  kkeep = 1
endif
if timeinsts() > 0.25 then
  kcoefs = 80
endif
akeep mincer apos, 1, 2, itab, 2, 512, 4, 1, kkeep, 8
ak mincer apos, 1, 2, itab, 2, 512, 4, 1, kkeep, kcoefs
check_equal_window ak, aoff, 1e-7, 1e-6, 0.04, 0.09, "initial off"
check_equal_window ak, adef, 1e-7, 1e-6, 0.04, 0.09, "initial def"
check_differs_window ak, aoff, 1e-5, 1e-5, 0.14, 0.23, "keep on"
check_equal_window ak, akeep, 1e-7, 1e-6, 0.14, 0.23, "coefs 8"
check_differs_window ak, akeep, 1e-6, 1e-6, 0.30, 0.44, "coefs change"
check_nonzero ak, 1e-6
kdone init 0
if timeinsts() > 0.4 && kdone == 0 then
  printks "formant k-rate ok\n", 0
  kdone = 1
endif
endin

instr 5
itab ftgen 0, 0, 0, 1, "formant_rate_22050.wav", 0, 0, 0
apos = 0
aminoff mincer apos, 1, 1, itab, 2, 512, 4
aminunity mincer apos, 1, 1, itab, 2, 512, 4, 1, 1, 80
aminupoff mincer apos, 1, 2, itab, 2, 512, 4
aminup mincer apos, 1, 2, itab, 2, 512, 4, 1, 1, 80
check_equal aminoff, aminunity, 1e-7, 1e-6
check_differs aminup, aminupoff, 1e-5, 1e-5
afileoff filescal 1, 1, 1, "formant_rate_22050.wav", 2, 512, 4, 1, 0, 1, 1
afileunity filescal 1, 1, 1, "formant_rate_22050.wav", 2, 512, 4, 1, 0, 1, 1, 1, 80
afileupoff filescal 1, 1, 2, "formant_rate_22050.wav", 2, 512, 4, 1, 0, 1, 1
afileup filescal 1, 1, 2, "formant_rate_22050.wav", 2, 512, 4, 1, 0, 1, 1, 1, 80
check_equal afileoff, afileunity, 1e-7, 1e-6
check_differs afileup, afileupoff, 1e-5, 1e-5
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "raw pitch bypass ok\n", 0
  kdone = 1
endif
endin

instr 6
itab ftgen 0, 0, 0, 1, "formant_sparse_44100.wav", 0, 0, 0
apos = 0
aoff mincer apos, 1, 0.5, itab, 0, 512, 4
am1 mincer apos, 1, 0.5, itab, 0, 512, 4, 1, 1, 80
am2 mincer apos, 1, 0.5, itab, 0, 512, 4, 1, 2, 80
check_differs am1, aoff, 1e-5, 1e-5
check_bounded am1, 5, 1e-6
check_bounded am2, 5, 1e-6
check_nonzero am2, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "sparse gain bounded ok\n", 0
  kdone = 1
endif
endin

instr 7
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
apos = 0
kCycle init 0
kk = kCycle < 32 ? 1 : 0
if kCycle == 32 then
  reinit FORMANT_RESET
endif
FORMANT_RESET:
aReset mincer apos, 1, 2, itab, 2, 512, 4, 1, kk, 80
rireturn
if kCycle >= 32 then
  aFresh mincer apos, 1, 2, itab, 2, 512, 4, 1, 0, 80
  check_equal aReset, aFresh, 1e-7, 1e-6
endif
kCycle += 1
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "formant state reset ok\n", 0
  kdone = 1
endif
endin

</CsInstruments>
<CsScore>
i1 0 0.4
i2 0 0.4
i3 0 0.4
i4 0 0.45
i5 0 0.4
i6 0 0.4
i7 0 0.4
</CsScore>
</CsoundSynthesizer>
