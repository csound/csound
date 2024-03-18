<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n 
</CsOptions>
<CsInstruments>
;example by joachim heintz

           seed       0

instr 1
;create an array with 10 elements
kArr[]     init       10
;fill in random numbers and print them out
kIndx      =          0
  until kIndx == 10 do
kNum       random     0, 10
kArr[kIndx] =         kNum
           printf     "kArr[%d] = %10f\n", kIndx+1, kIndx, kNum
kIndx      +=         1
  od
;calculate sum of all values and print it out
kSum       sumarray   kArr
           printf     "Sum of all values in kArr = %f\n", kIndx+1, kSum
           turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
