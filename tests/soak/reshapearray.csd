<CsoundSynthesizer>
<CsOptions>
;-odac     ;;;realtime audio out

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs  = 1

; This is the example file for reshapearray

/*

reshapearray 

  Reshape an array, maintaining the capacity of the array
  (it does NOT resize the array).

  You can reshape a 2D array to another array of equal capacity
  of reshape a 1D array to a 2D array, or a 2D array to a 1D
  array

  reshapearray array[], inumrows, inumcols=0

  works with i and k arrays, at i-time and k-time

*/


instr 1
  ivalues[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
  printarray ivalues
  reshapearray ivalues, 3, 4
  printarray ivalues
  turnoff
endin

instr 2
  kxs[][] init 3, 4
  kxs fillarray   0,  1,  2,  3, \
                 10, 11, 12, 13, \
                 20, 21, 22, 23
  reshapearray kxs, 4, 3
  printarray kxs, 1, "", "kxs after"
  turnoff
endin

instr 3
  kxs[][] init 3, 4
  kxs fillarray   0,  1,  2,  3, \
                 10, 11, 12, 13, \
                 20, 21, 22, 23
  reshapearray kxs, 12
  printarray kxs, 1, "", "kxs after"
  turnoff

endin

</CsInstruments>

<CsScore>
i 1 0 0.01
i 2 1 0.01
i 3 2 0.01
</CsScore>

</CsoundSynthesizer>
