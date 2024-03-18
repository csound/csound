<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

;create and fill two arrays
iArr1[] genarray 1, 6
iArr2[] genarray 1, 6, 2/3

;print the content of iArr1
        printf  "%s", 1, "iArr1: start=1, end=6, step=default\n"
kndx    =       0
  until kndx == lenarray(iArr1) do
        printf  "iArr[%d] = %f\n", kndx+1, kndx, iArr1[kndx]
kndx    +=      1
  od

;print the content of iArr2
        printf  "%s", 1, "iArr2: start=1, end=6, step=2/3\n"
kndx    =       0
  until kndx == lenarray(iArr2) do
        printf  "iArr[%d] = %f\n", kndx+1, kndx, iArr2[kndx]
kndx    +=      1
  od

        turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>

