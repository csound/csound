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

;8 points sine wave function table
giSine  ftgen   0, 0, 8, 10, 1


  instr 1
;create array
kArr[]  init    8

;copy table values in it
        copyf2array kArr, giSine
        
;print values
kndx    =       0
  until kndx == lenarray(kArr) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
kndx    +=      1
  od

;turn instrument off
        turnoff
  endin
  
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
