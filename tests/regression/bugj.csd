<CsoundSynthesizer>

<CsInstruments>

instr 1
SArr[] fillarray "a", "b", "c"
;;b) not possible to have arrays as arguments, e.g.
    iArr1[] fillarray 1, 2, 3
    iArr2[] fillarray 4, 5, 6
;;***    iArr[][] fillarray iArr1, iArr2

;;2) lenarray
    Sarr[] init 3
    print lenarray(Sarr)

;;3) slicearray
   iArr[]  fillarray  1, 2, 3, 4, 5, 6, 7, 8, 9
   iArr1[] init       5
   iArr2[] init       4
iArr1   slicearray iArr, 0, 4        ;[1, 2, 3, 4, 5]
iArr2   slicearray iArr, 5, 8        ;[6, 7, 8, 9]
;;b) same for S-arrays:
   SArr[]  init       9
   SArr1[] init       5
;;***   SArr1   slicearray SArr, 0, 4

;;4) copyf2array
    giSine  ftgen   0, 0, 8, 10, 1
    iArr[]  init       8
    copyf2array iArr, giSine

;;5) copya2ftab
;;***    copya2ftab giSine, iArr

;;6) +, -, *, / between an array and a number
;;do not work at i-rate. this works:
    kArr1[] fillarray 1, 2, 3
    kArr2[] = kArr1 + 10
;;but this does not:
    iArr1[]  array 1, 2, 3
    iArr2[] = iArr1 + 10
    print iArr1[0], iArr1[1], iArr1[2]
    print iArr2[0], iArr2[1], iArr2[2]

;;7) +, -, *, / between two arrays
;;also missing for i. this works:
    kArr1[] fillarray 1, 2, 3
    kArr2[] fillarray 10, 20, 30
    kArr3[] = kArr1 + kArr2
;;but this does not:
    iArr1[] fillarray 1, 2, 3
    iArr2[] fillarray 10, 20, 30
    iArr3[] = iArr1 + iArr2

;;8) minarray, maxarray, sumarray, scalearray
;;do all miss i-time operations, as far as i see.
    ii minarray  iArr1
    ij maxarray  iArr1
    ik sumarray  iArr1
       scalearray iArr1, 10, 20

;;9) maparray
;;as well:
    iArrSrc[] array 1.01, 2.02, 3.03, 4.05, 5.08, 6.13, 7.21
    iArrRes[] init  7
    iArrRes maparray iArrSrc, "sqrt"

;;10) =
;;miss i-time operations. this is possible:
    kArr1[] fillarray 1, 2, 3
    kArr2[] = kArr1
;;but this returns "nul opadr":
    iArr1[] fillarray 1, 2, 3
   iArr2[] = iArr1

endin
</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>

</CsoundSynthesizer>
