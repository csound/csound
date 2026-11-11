<CsTest>
description = "phase-locking modes for mincer, temposcal, and filescal"

[expect]
exit = 0
output = ["mincer ok", "temposcal ok", "filescal ok"]
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
if timeinsts() > 0.2 && (kdiff > ktol || klevel < kmin) then
  printks "equal check failed %.9f %.9f\n", 1, kdiff, klevel
  exitnowk(-1)
endif
endop

opcode check_differs, 0, aakk
a1, a2, ktol, kmin xin
kdiff rms (a1 - a2)
if timeinsts() > 0.2 && kdiff < kmin then
  printks "difference check failed %.9f\n", 1, kdiff
  exitnowk(-1)
endif
endop

opcode check_nonzero, 0, ak
a, kmin xin
klevel rms a
if timeinsts() > 0.2 && klevel < kmin then
  printks "level check failed %.9f\n", 1, klevel
  exitnowk(-1)
endif
endop

instr 1
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
apos = 0.1
a3def mincer apos, 1, 1, itab, 3, 512, 4
a3one mincer apos, 1, 1, itab, 3, 512, 4, 1
a3half mincer apos, 1, 1, itab, 3, 512, 4, 0.5
a3two mincer apos, 1, 1, itab, 3, 512, 4, 2
a2 mincer apos, 1, 1, itab, 2, 512, 4
check_equal a3def, a3one, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_nonzero a2, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "mincer ok\n", 0
  kdone = 1
endif
endin

instr 2
itab ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 0
a3def temposcal 1, 1, 1, itab, 3, 512, 4
a3one temposcal 1, 1, 1, itab, 3, 512, 4, 1, 0, 1, 1
a3half temposcal 1, 1, 1, itab, 3, 512, 4, 1, 0, 1, 0.5
a3two temposcal 1, 1, 1, itab, 3, 512, 4, 1, 0, 1, 2
a2 temposcal 1, 1, 1, itab, 2, 512, 4
check_equal a3def, a3one, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_nonzero a2, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "temposcal ok\n", 0
  kdone = 1
endif
endin

instr 3
a3def filescal 1, 1, 1, "fox.wav", 3, 512, 4
a3one filescal 1, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 1
a3half filescal 1, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 0.5
a3two filescal 1, 1, 1, "fox.wav", 3, 512, 4, 1, 0, 1, 2
a2 filescal 1, 1, 1, "fox.wav", 2, 512, 4
check_equal a3def, a3one, 1e-7, 1e-6
check_differs a3half, a3two, 1e-5, 1e-5
check_nonzero a2, 1e-6
check_nonzero a3def, 1e-6
kdone init 0
if timeinsts() > 0.3 && kdone == 0 then
  printks "filescal ok\n", 0
  kdone = 1
endif
endin

</CsInstruments>
<CsScore>
i1 0 0.4
i2 0 0.4
i3 0 0.4
</CsScore>
</CsoundSynthesizer>
