<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n 
</CsOptions>
<CsInstruments>
ksmps = 32
;example by joachim heintz

           seed       0

instr 1
;create an array with 10 elements
kArr[]     init       10
;fill in random numbers and print them out
kIndx      =          0
  until kIndx == 10 do
kNum       random     -100, 100
kArr[kIndx] =         kNum
           printf     "kArr[%d] = %10f\n", kIndx+1, kIndx, kNum
kIndx      +=         1
  od
;investigate minimum number and print it out
kMin, kMinIndx minarray kArr
           printf     "Minimum of kArr = %f at index %d\n", kIndx+1, kMin, kMinIndx
           turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
