<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=64
ksmps=64
nchnls=1
0dbfs=1

opcode OscBank,a,kki
setksmps 1
kfr,kamp,inum xin
au init 0
kph[] init inum
kcnt init 0

until kcnt == inum do
au += sin(kph[kcnt])       ; this line segfaults
kph[kcnt] = kph[kcnt] + kfr*kcnt*(2*$M_PI)/sr  ; this line segfaults
kcnt += 1
od
xout au*kamp
au = 0
kcnt = 0

endop


instr 1

aout OscBank 440, .5, 8

out aout

endin

opcode PassAudioArray1, a[], a[]
  aIn[][] xin
  setksmps 1
  xout aIn
endop

opcode PassAudioArray32, a[], a[]
  aIn[][] xin
  setksmps 32
  xout aIn
endop

instr 2
  aIn[][] init 2, 3
  aIn[0][0] = 0.1
  aIn[0][1] = 0.2
  aIn[0][2] = 0.3
  aIn[1][0] = 0.4
  aIn[1][1] = 0.5
  aIn[1][2] = 0.6

  aOut1[][] PassAudioArray1 aIn
  aOut32[][] PassAudioArray32 aIn

  aError1 = abs(aOut1[0][0] - aIn[0][0]) \
          + abs(aOut1[0][1] - aIn[0][1]) \
          + abs(aOut1[0][2] - aIn[0][2]) \
          + abs(aOut1[1][0] - aIn[1][0]) \
          + abs(aOut1[1][1] - aIn[1][1]) \
          + abs(aOut1[1][2] - aIn[1][2])
  aError32 = abs(aOut32[0][0] - aIn[0][0]) \
           + abs(aOut32[0][1] - aIn[0][1]) \
           + abs(aOut32[0][2] - aIn[0][2]) \
           + abs(aOut32[1][0] - aIn[1][0]) \
           + abs(aOut32[1][1] - aIn[1][1]) \
           + abs(aOut32[1][2] - aIn[1][2])
  kError1 downsamp aError1
  kError32 downsamp aError32
  if kError1 > 0.0000001 || kError32 > 0.0000001 then
    printks "multidimensional audio array UDO copy failed: %f, %f\n", \
            0, kError1, kError32
    exitnowk(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.5
i 2 0 0.5
</CsScore>
</CsoundSynthesizer>

