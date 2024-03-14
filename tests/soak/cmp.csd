<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 5
0dbfs = 1.0

; this is the example file for 'cmp'

/*

cmp

compare audio or arrays, value by value

Audio:
  * compare audio signals, sample by sample, against another signal or scalar
  * compara audio singal, sample by sample, within a range:
    aout cmp klo, "<", ain, "<=", khi

Arrays:
  * compare arrays value by value or against a scalar
  * compare array value by value within a range:
    kout[] = klo < kin[] <= khi   ->   kout[] cmp klo, "<", kin[], "<=", khi

aout cmp a1, Sop, a2                 : aout cmp ain, "<", acmp
aout cmp a1, Sop, kval               : aout cmp ain, ">=", 0.1
kout[] cmp k1[], Sop, k2[]           : kout[] cmp kxs, "<", kys
iout[] cmp i1[], Sop, i2[]           : iout[] cmp ixs, "<", iys
kout[] cmp k1[], Sop, k              : kout[] cmp kxs, "<", 0.5
iout[] cmp i1[], Sop, i              : iout[] cmp ixs, "<", 0.5
kout[] cmp klo, Sop, kx[], Sop, khi  : kout[] cmp 0, "<", kxs, "<=", 1
iout[] cmp ilo, Sop, ix[], Sop, ihi  : iout[] cmp 0, "<", ixs, "<=", 1

TODO: implement array operations for multidim. arrays
      (at the time, array operations work only for 1D-arrays)

*/

; for audio operations, render this to a soundfile and open in an editor
; to check the results

instr 1
  a0 linseg 0, p3, 1
  a1 linseg 1, p3, 0
  aout1 cmp a0, "<", a1
  aout2 cmp a0, "<=", 0.5
  aout3 cmp a0, ">", 0.5
  outch 1, a0
  outch 2, a1
  outch 3, aout1
  outch 4, aout2
  outch 5, aout3
endin

instr 4
  ; cmp with arrays
  ixs[] fillarray 0, 1, 2, 3, 4, 5
  iys[] cmp ixs, ">=", 3
  printarray iys, "", "instr 4, iys"
  
  kxs[] fillarray 0, 1, 2, 3, 4, 5
  kys[] cmp kxs, ">=", 3
  printarray kys, 1, "", "instr 4, kys"
  turnoff
endin

instr 5
  ; range
  ixs[] fillarray 0, 1, 2, 3, 4, 5
  iys[] cmp 1, "<", ixs, "<=", 4
  printarray iys, "", "instr 5, iys"

  kxs[] fillarray 0, 1, 2, 3, 4, 5
  kys[] cmp 1, "<", kxs, "<=", 4
  printarray kys, 1, "", "instr 5, kys"
  turnoff
endin



</CsInstruments>
<CsScore>
i 1 0 2
i 4 0 1
i 5 0 1
</CsScore>
</CsoundSynthesizer>

