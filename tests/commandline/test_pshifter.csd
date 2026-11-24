<CsTest>
description = "live-input pitch shifter with formant scaling"

[expect]
exit = 0
output = ["pshifter ok"]
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
if timeinsts() > 0.2 && !(kdiff >= ktol && kdiff < 1e30) then
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
itab ftgen 0, 0, 4096, 10, 1, 0.4, 0.2, 0.1, 0.05, 0.02
ain = 0.18*oscili(1, 220, itab) + 0.09*oscili(1, 440, itab)
aunit, idel pshifter ain, 1, 1, 0, 0, 512, 4
apitch, idel2 pshifter ain, 1.4, 1, 0, 0, 512, 4
afscal0, idel3 pshifter ain, 1.4, 2, 0, 0, 512, 4
aform1, idel4 pshifter ain, 1.4, 1, 0, 1, 512, 4, 80
aform2, idel5 pshifter ain, 1.4, 2, 0, 1, 512, 4, 80
alock1, idel6 pshifter ain, 1, 1, 1, 0, 512, 4
alock2, idel7 pshifter ain, 1, 1, 2, 0, 512, 4
alock3, idel8 pshifter ain, 1, 1, 3, 0, 512, 4
aclamp, idel9 pshifter ain, 1e30, -1e30, -3, 2, 512, 4, 80

if idel != 512/sr || idel != idel2 || idel2 != idel3 || idel3 != idel4 || idel4 != idel5 || idel5 != idel6 || idel6 != idel7 || idel7 != idel8 || idel8 != idel9 then
  printks "delay check failed %.9f %.9f\n", 1, idel, idel9
  exitnow(-1)
endif

check_nonzero aunit, 1e-4
check_differs apitch, aunit, 1e-5, 1e-5
check_equal afscal0, apitch, 1e-9, 1e-4
check_differs aform2, aform1, 1e-5, 1e-5
check_differs alock1, aunit, 1e-5, 1e-5
check_equal alock3, alock2, 1e-7, 1e-4
check_nonzero aclamp, 1e-4

kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "pshifter ok\n", 0
  kdone = 1
endif
endin

</CsInstruments>
<CsScore>
i1 0 0.5
</CsScore>
</CsoundSynthesizer>
