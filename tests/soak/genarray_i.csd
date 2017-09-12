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
kArr1[] genarray_i 1, 6
kArr2[] genarray_i 1, 6, 2/3

;print the content of kArr1
        printf  "%s", 1, "kArr1: start=1, end=6, step=default\n"
kndx    =       0
  until kndx == lenarray(kArr1) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr1[kndx]
kndx    +=      1
  od

;print the content of kArr2
        printf  "%s", 1, "kArr2: start=1, end=6, step=2/3\n"
kndx    =       0
  until kndx == lenarray(kArr2) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr2[kndx]
kndx    +=      1
  od

        turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>

