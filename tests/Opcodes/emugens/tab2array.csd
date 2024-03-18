<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Example file tab2array

/*

tab2array: copy a slice of a table to an array

kout[] tab2array ifn, kstart=0, kend=0, kstep=1
iout[] tab2array ifn, istart=0, iend=0, istep=1

ifn: the table index to copy from
start: the index to start copying from
end: the end index to stop copying. This is NOT inclusive. 0=end of table
step: how many elements to skip

*/

instr 1
  ifn ftgentmp 0,0,-13,-2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
  ; copy everything at i-time, similar to copyf2array, but no need
  ; to predefine the array
  islice[] tab2array ifn
  printarray islice, "", "islice"
  
  ; copy the slice [1::2] to an array, at k-time
  kslice[] tab2array ifn, 1, 0, 2
  printarray kslice, 1, "", "kslice"

  ; copy into a predefined array. If the number of elements to copy
  ; excede the capacity of the array, the array is enlarged  
  kxs[] init 3
  kxs tab2array ifn, 0, 10
  printarray kxs, 1, "", "kxs"
  turnoff
endin


</CsInstruments>
<CsScore>
i 1 0   0.1

</CsScore>
</CsoundSynthesizer>
