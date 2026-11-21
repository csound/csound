<CsTest>
description = "phase-locking modes for mincer, temposcal, and filescal"

[expect]
exit = 0
output = ["mincer ok", "temposcal ok", "filescal ok", "synthetic ok"]
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
a0 mincer apos, 1, 1, itab, 0, 512, 4
a1 mincer apos, 1, 1, itab, 1, 512, 4
a2 mincer apos, 1, 1, itab, 2, 512, 4
a3def mincer apos, 1, 1, itab, 3, 512, 4
a3one mincer apos, 1, 1, itab, 3, 512, 4, 1
a3half mincer apos, 1, 1, itab, 3, 512, 4, 0.5
a3two mincer apos, 1, 1, itab, 3, 512, 4, 2
a1half mincer apos, 1, 1, itab, 0.5, 512, 4
a1neg mincer apos, 1, 1, itab, -0.5, 512, 4
a2half mincer apos, 1, 1, itab, 2.5, 512, 4
check_nonzero a0, 1e-6
check_nonzero a1, 1e-6
check_differs a2, a0, 1e-5, 1e-5
check_equal a3def, a3one, 1e-7, 1e-6
check_equal a3one, a2, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_equal a1half, a1, 1e-7, 1e-6
check_equal a1neg, a1, 1e-7, 1e-6
check_equal a2half, a1, 1e-7, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "mincer ok\n", 0
  kdone = 1
endif
endin

instr 2
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
a0 temposcal 0.5, 1, 1, itab, 0, 512, 4
a1 temposcal 0.5, 1, 1, itab, 1, 512, 4
a2 temposcal 0.5, 1, 1, itab, 2, 512, 4
a3def temposcal 0.5, 1, 1, itab, 3, 512, 4
a3one temposcal 0.5, 1, 1, itab, 3, 512, 4, 1, 0, 1, 1
a3half temposcal 0.5, 1, 1, itab, 3, 512, 4, 1, 0, 1, 0.5
a3two temposcal 0.5, 1, 1, itab, 3, 512, 4, 1, 0, 1, 2
a1half temposcal 0.5, 1, 1, itab, 0.5, 512, 4
a1neg temposcal 0.5, 1, 1, itab, -0.5, 512, 4
a2half temposcal 0.5, 1, 1, itab, 2.5, 512, 4
check_nonzero a0, 1e-6
check_nonzero a1, 1e-6
check_differs a2, a0, 1e-5, 1e-5
check_equal a3def, a3one, 1e-7, 1e-6
check_equal a3one, a2, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_equal a1half, a1, 1e-7, 1e-6
check_equal a1neg, a1, 1e-7, 1e-6
check_equal a2half, a1, 1e-7, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "temposcal ok\n", 0
  kdone = 1
endif
endin

instr 3
a0 filescal 0.5, 1, 1, "fox.wav", 0, 512, 4
a1 filescal 0.5, 1, 1, "fox.wav", 1, 512, 4
a2 filescal 0.5, 1, 1, "fox.wav", 2, 512, 4
a3def filescal 0.5, 1, 1, "fox.wav", 3, 512, 4
a3one filescal 0.5, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 1
a3half filescal 0.5, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 0.5
a3two filescal 0.5, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 2
a1half filescal 0.5, 1, 1, "fox.wav", 0.5, 512, 4
a1neg filescal 0.5, 1, 1, "fox.wav", -0.5, 512, 4
a2half filescal 0.5, 1, 1, "fox.wav", 2.5, 512, 4
check_nonzero a0, 1e-6
check_nonzero a1, 1e-6
check_differs a2, a0, 1e-5, 1e-5
check_equal a3def, a3one, 1e-7, 1e-6
check_equal a3one, a2, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_equal a1half, a1, 1e-7, 1e-6
check_equal a1neg, a1, 1e-7, 1e-6
check_equal a2half, a1, 1e-7, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "filescal ok\n", 0
  kdone = 1
endif
endin

instr 4
itab1 ftgen 0, 0, 4096, 10, 0.5, 0.3, 0.2, 0.1, 0.05
itab2 ftgen 0, 0, 4096, 10, -0.5, -0.3, -0.2, -0.1, -0.05
apos = 0.01
a2 mincer apos, 1, 8, itab1, 2, 512, 4
a3one mincer apos, 1, 8, itab1, 3, 512, 4, 1
a3half mincer apos, 1, 8, itab1, 3, 512, 4, 0.5
a3two mincer apos, 1, 8, itab1, 3, 512, 4, 2
aneg mincer apos, 1, 8, itab2, 3, 512, 4, 0.5
check_equal a3one, a2, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_equal aneg, -a3half, 1e-7, 1e-6
check_nonzero a3half, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "synthetic ok\n", 0
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
