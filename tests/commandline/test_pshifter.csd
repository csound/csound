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

giSine ftgen 1, 0, 4096, 10, 1

opcode check_tone, 0, aiiiii
asig, imin, itarget, isource, istart, iratio xin
acos oscili 1, itarget, giSine, 0.25
asrc oscili 1, isource, giSine, 0.25
atarget butlp asig*acos, 10
asource butlp asig*asrc, 10
ktarget rms atarget
ksource rms asource
if timeinsts() > istart && !(ktarget >= imin && ktarget > iratio*ksource) then
  printks "tone check failed %.9f %.9f\n", 1, ktarget, ksource
  exitnowk(-1)
endif
endop

opcode check_no_tone, 0, aii
asig, ifreq, istart xin
acos oscili 1, ifreq, giSine, 0.25
ares butlp asig*acos, 10
kres rms ares
if timeinsts() > istart && kres > 1e-3 then
  printks "absent tone check failed %.9f\n", 1, kres
  exitnowk(-1)
endif
endop

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
adry delayr idel
      delayw ain
asine = 0.5*oscili(1, 110, giSine)
apure, idel10 pshifter asine, 1.4, 1, 0, 0, 512, 4
kshift linseg 0.5, 0.3, 0.5, 0.2, 1, 0.25, 1
achanged, idel11 pshifter asine, kshift, 1, 0, 0, 512, 4
aoctsource = 0.2*oscili(1, 344.53125, giSine)
aoct, idel12 pshifter aoctsource, 2, 1, 0, 0, 512, 4
aconst = 0.5
aconst4, idel13 pshifter aconst, 1, 1, 0, 0, 512, 4
aconst8, idel14 pshifter aconst, 1, 1, 0, 0, 512, 8
aconst16, idel15 pshifter aconst, 1, 1, 0, 0, 512, 16
adefault, idel20 pshifter aconst, 1, 1, 0, 0, 512
adry4 delayr idel13
      delayw aconst
adry8 delayr idel14
      delayw aconst
adry16 delayr idel15
      delayw aconst
abin19 = 0.2*oscili(1, 19*sr/512, giSine)
adown, idel16 pshifter abin19, 0.25, 1, 0, 0, 512, 4
abin1000 = 0.2*oscili(1, 1000, giSine)
aup, idel17 pshifter abin1000, 4, 1, 0, 0, 512, 4
aalt = 0.2*oscili(1, sr/2, giSine, 0.25)
anyq, idel18 pshifter aalt, 0.5, 1, 0, 0, 512, 4
aupnyq, idel19 pshifter aalt, 2, 1, 0, 0, 512, 4

if idel != 512/sr || idel != idel2 || idel2 != idel3 || idel3 != idel4 || idel4 != idel5 || idel5 != idel6 || idel6 != idel7 || idel7 != idel8 || idel8 != idel9 || idel9 != idel10 || idel10 != idel11 || idel11 != idel12 || idel12 != idel13 || idel13 != idel14 || idel14 != idel15 || idel15 != idel20 || idel20 != idel16 || idel16 != idel17 || idel17 != idel18 || idel18 != idel19 then
  printks "delay check failed %.9f %.9f\n", 1, idel, idel19
  exitnow(-1)
endif

check_equal aunit, adry, 1e-6, 1e-4
check_nonzero aunit, 1e-4
check_differs apitch, aunit, 1e-5, 1e-5
check_equal afscal0, apitch, 1e-6, 1e-4
check_differs aform2, aform1, 1e-5, 1e-5
check_equal alock1, aunit, 1e-6, 1e-4
check_equal alock2, aunit, 1e-6, 1e-4
check_equal alock3, alock2, 1e-6, 1e-4
check_nonzero aclamp, 1e-4
check_nonzero apure, 1e-4
check_tone apure, 0.02, 154, 110, 0.3, 8
check_nonzero achanged, 1e-4
check_tone achanged, 0.02, 110, 55, 0.8, 8
check_nonzero aoct, 1e-4
check_tone aoct, 0.01, 689.0625, 344.53125, 0.3, 2
check_equal aconst4, adry4, 1e-6, 1e-4
check_equal aconst8, adry8, 1e-6, 1e-4
check_equal aconst16, adry16, 1e-6, 1e-4
check_equal adefault, aconst8, 1e-6, 1e-4
check_nonzero adown, 1e-4
check_tone adown, 0.01, 409.130859, 1636.523438, 0.3, 8
check_nonzero aup, 1e-4
check_tone aup, 0.005, 4000, 1000, 0.3, 4
check_tone anyq, 0.01, 11025, 22050, 0.3, 8
check_no_tone aupnyq, 22050, 0.3

kdone init 0
if timeinsts() > 0.9 && kdone == 0 then
  printks "pshifter ok\n", 0
  kdone = 1
endif
endin

</CsInstruments>
<CsScore>
i1 0 1.0
</CsScore>
</CsoundSynthesizer>
