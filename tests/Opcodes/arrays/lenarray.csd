<CsoundSynthesizer>
<CsOptions>
-n -m128 ;no sound output, reduced messages
</CsOptions>
<CsInstruments>
;example by joachim heintz
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

  instr 1 ;simple example
kArr[]  fillarray 1, 2, 3, 4 ;fill array manually
        printks "Length of kArr = %d\n\n", 0, lenarray(kArr) ;print out its length
        turnoff ;only do this in the first k-cycle
  endin

  instr 2 ;random array length
iNumEls random  1, 11 ;create random number between 1 and 10
kArr[]  init    int(iNumEls) ;create array of this length
        printks "Random length of kArr = %d\n", 0, lenarray(kArr) ;print out
        turnoff
  endin

  instr 3 ;fill random array length with random elements
iNumEls random  1, 11 ;create random number between 1 and 10
kArr[]  init    int(iNumEls) ;create array of this length
        printks "Random length of kArr = %d\n", 0, lenarray(kArr) ;print out

;fill
kIndx   =       0 ;initialize index
  until kIndx == lenarray(kArr) do
kArr[kIndx] rnd31 10, 0 ;set element to random value -10...10
kIndx   +=      1 ;increase index
  od

;print
kIndx   =       0 ;initialize index
  until kIndx == lenarray(kArr) do
printf("kArr[%d] = %f\n", kIndx+1, kIndx, kArr[kIndx])
kIndx   +=      1 ;increase index
  od

        turnoff
  endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 0 .1 
i 2 0 .1
i 3 0 .1
</CsScore>
</CsoundSynthesizer>
