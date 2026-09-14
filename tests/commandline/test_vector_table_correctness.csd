<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1

giRead ftgen 1, 0, 8, -2, 10, 11, 20, 21, 30, 31, 40, 41
giVtableWrite ftgen 2, 0, 8, -2, 0, 0, 0, 0, 0, 0, 0, 0
giVtabWrite ftgen 3, 0, 8, -2, 0, 0, 0, 0, 0, 0, 0, 0
giSelect1 ftgen 4, 0, 4, -2, 100, 101, 102, 103
giSelect2 ftgen 5, 0, 4, -2, 200, 201, 202, 203
giGuard ftgen 6, 0, 9, -7, 0, 8, 8

instr 1
  iout1 init 0
  iout2 init 0
  vtablei 3, giRead, 1, 0, iout1, iout2
  if abs(iout1 - 40) > .000001 || abs(iout2 - 41) > .000001 then
    prints "vtablei could not read the final vector\n"
    exitnow(-1)
  endif
  vtablei 3.5, giRead, 1, 0, iout1, iout2
  if abs(iout1 - 25) > .000001 || abs(iout2 - 26) > .000001 then
    prints "vtablei did not interpolate the final vector correctly\n"
    exitnow(-1)
  endif
  vtablei 7.5, giGuard, 1, 0, iout1
  if abs(iout1 - 7.5) > .000001 then
    prints "vtablei changed scalar guard-point interpolation\n"
    exitnow(-1)
  endif
endin

instr 2
  andx = 5
  anormal = 1.375
  avtable1 init 0
  avtable2 init 0
  avtab1 init 0
  avtab2 init 0
  ainterp1 init 0
  ainterp2 init 0
  vtablea andx, giRead, 0, 0, avtable1, avtable2
  vtaba andx, giRead, avtab1, avtab2
  vtablea anormal, giRead, 1, 1, ainterp1, ainterp2

  kvt1 max_k avtable1, 1, 1
  kvt2 max_k avtable2, 1, 1
  kt1 max_k avtab1, 1, 1
  kt2 max_k avtab2, 1, 1
  ki1 max_k ainterp1, 1, 1
  ki2 max_k ainterp2, 1, 1
  kfirst init 1
  if kfirst == 1 then
    if abs(kvt1 - 20) > .000001 || abs(kvt2 - 21) > .000001 || \
       abs(kt1 - 20) > .000001 || abs(kt2 - 21) > .000001 || \
       abs(ki1 - 25) > .000001 || abs(ki2 - 26) > .000001 then
      printks "vector table read used an inactive index sample\n", 0
      exitnowk(-1)
    endif
    kfirst = 0
  endif
endin

instr 3
  andx = 5
  ain1 = 60
  ain2 = 61
  vtablewa andx, giVtableWrite, 0, ain1, ain2
  vtabwa andx, giVtabWrite, ain1, ain2
endin

instr 4
  kfn init 4.4
  kcount init 0
  if kcount > 0 then
    kfn = 4.6
  endif
  kout init 0
  vtablek 5.5, kfn, 1, 0, kout
  if kcount == 1 && abs(kout - 201.5) > .000001 then
    printks "vtablek did not follow the selected table number\n", 0
    exitnowk(-1)
  endif
  kcount += 1
endin

instr 99
  ivtable0 table 0, giVtableWrite
  ivtable1 table 1, giVtableWrite
  ivtable2 table 2, giVtableWrite
  ivtable3 table 3, giVtableWrite
  ivtab0 table 0, giVtabWrite
  ivtab1 table 1, giVtabWrite
  ivtab2 table 2, giVtabWrite
  ivtab3 table 3, giVtabWrite

  if abs(ivtable0) > .000001 || abs(ivtable1) > .000001 || \
     abs(ivtable2 - 60) > .000001 || abs(ivtable3 - 61) > .000001 || \
     abs(ivtab0) > .000001 || abs(ivtab1) > .000001 || \
     abs(ivtab2 - 60) > .000001 || abs(ivtab3 - 61) > .000001 then
    prints "vector table write used an inactive index sample\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 .021875 .001
i 3 .041875 .000125
i 4 .06 .006
i 99 .08 .001
</CsScore>
</CsoundSynthesizer>
