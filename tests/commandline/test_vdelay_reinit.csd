<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  kcount init 0
  asignal = kcount / 1000
  kdelay = .1875
  adelay = kdelay
  aref vdelay3 asignal, kdelay, 1
  if kcount == 16 || kcount == 32 then
    reinit DELAY
  endif
DELAY:
  ; Skipping initialization must preserve the buffer size and write position.
  imax = i(kcount) == 0 ? 1 : (i(kcount) == 16 ? 100 : .25)
  iskip = i(kcount) == 0 ? 0 : 1
  ak vdelay asignal, kdelay, imax, iskip
  aa vdelay asignal, adelay, imax, iskip
  ak3 vdelay3 asignal, kdelay, imax, iskip
  aa3 vdelay3 asignal, adelay, imax, iskip
  rireturn
  if kcount > 8 then
    kref downsamp aref
    kk downsamp ak
    ka downsamp aa
    kk3 downsamp ak3
    ka3 downsamp aa3
    if !(abs(kk-kref) < .00001 && abs(ka-kref) < .00001 && abs(kk3-kref) < .00001 && abs(ka3-kref) < .00001) then
      printks "vdelay reinit mismatch at sample %g: ref=%g k=%g a=%g k3=%g a3=%g\n", 0, kcount, kref, kk, ka, kk3, ka3
      exitnowk(-1)
    endif
  endif
  if kcount == 100 then
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 1 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
