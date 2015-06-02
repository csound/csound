<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
ksmps = 32

/****UDOs for printing one-dim arrays****/
  opcode PrtArr1i, 0, i[]ojjj
iArr[], istart, iend, iprec, ippr xin
iprint     init       0
ippr       =          (ippr == -1 ? 10 : ippr)
iend       =          (iend == -1 ? lenarray(iArr) : iend)
iprec      =          (iprec == -1 ? 3 : iprec)
indx       =          istart
Sformat    sprintf    "%%%d.%df, ", iprec+3, iprec
Sdump      sprintf    "%s", "["
loop:
Snew       sprintf    Sformat, iArr[indx]
Sdump      strcat     Sdump, Snew
imod       =          (indx+1-istart) % ippr
 if imod == 0 && indx != iend-1 then
           printf_i   "%s\n", iprint+1, Sdump
Sdump      strcpy     " "
 endif
iprint     =          iprint + 1
           loop_lt    indx, 1, iend, loop
ilen       strlen     Sdump
Slast      strsub     Sdump, 0, ilen-2
           printf_i   "%s]\n", iprint+1, Slast
  endop
  opcode PrtArr1k, 0, k[]POVVO
kArr[], ktrig, kstart, kend, kprec, kppr xin
kprint     init       0
kndx       init       0
if ktrig > 0 then
kppr       =          (kppr == 0 ? 10 : kppr)
kend       =          (kend == -1 || kend == .5 ? lenarray(kArr) : kend)
kprec      =          (kprec == -1 || kprec == .5 ? 3 : kprec)
kndx       =          kstart
Sformat    sprintfk   "%%%d.%df, ", kprec+3, kprec
Sdump      sprintfk   "%s", "["
loop:
Snew       sprintfk   Sformat, kArr[kndx]
Sdump      strcatk    Sdump, Snew
kmod       =          (kndx+1-kstart) % kppr
 if kmod == 0 && kndx != kend-1 then
           printf     "%s\n", kprint+1, Sdump
Sdump      strcpyk    " "
 endif
kprint     =          kprint + 1
           loop_lt    kndx, 1, kend, loop
klen       strlenk    Sdump
Slast      strsubk    Sdump, 0, klen-2
           printf     "%s]\n", kprint+1, Slast
endif
  endop
  opcode PrtArr1S, 0, S[]oj
SArr[], istart, iend xin
iend       =          (iend == -1 ? lenarray(SArr) : iend)
indx       =          istart
           printf_i   "%s", 1, "["
 until indx >= iend-1 do
           printf_i    "%s, ", 1, SArr[indx]
indx       +=         1
 enduntil
           printf_i   "%s]\n", 1, SArr[indx]
  endop
  
  

    giSine  ftgen   0, 0, 8, 10, 1

instr Fillarray
    prints "\n instr Fillarray\n"
;different types, one dim
    iArr1d[] fillarray 1, 2, 3
    kArr1d[] fillarray 1, 2, 3
    SArr1d[] fillarray "a", "b", "c"
    prints "iArr1d = "
    PrtArr1i iArr1d
    prints "SArr1d = "
    PrtArr1S SArr1d
    printks "kArr1d = ", 0
    PrtArr1k kArr1d
;different types, two dims
    iArr2d[][] init 2, 3
    iArr2d fillarray 1, 2, 3, 4, 5, 6
    kArr2d[][] init 3, 2
    kArr2d fillarray 1, 2, 3, 4, 5, 6
    SArr2d[] init 2, 3
    SArr2d fillarray "a", "b", "c", "e", "f", "g"
;two dims, arrays as arguments
;    iArr1[] fillarray 1, 2, 3
;    iArr2[] fillarray 4, 5, 6
;    iArrcmb[][] fillarray iArr1, iArr2
    turnoff
endin


instr Lenarray
    prints "\n instr Lenarray\n"
    iArr1d[] fillarray 1, 2, 3
    kArr1d[] fillarray 1, 2, 3
    SArr1d[] fillarray "a", "b", "c"
    prints "Length of iArr1d = %d\n", lenarray(iArr1d)
    prints "Length of SArr1d = %d\n", lenarray(SArr1d)
    printks "Length of kArr1d = %d\n", 0, lenarray(kArr1d)
    turnoff    
endin


instr Slicearray
    prints "\n instr Sclicearray\n"
;i-rate
   iArr[]  fillarray  1, 2, 3, 4, 5, 6, 7, 8, 9
   iArr1[] init       5
   iArr2[] init       4
   iArr1   slicearray iArr, 0, 4
   iArr2   slicearray iArr, 5, 8
           PrtArr1i   iArr1
           PrtArr1i   iArr2
;k-rate
   kArr[]  fillarray  1, 2, 3, 4, 5, 6, 7, 8, 9
   kArr1[] init       5
   kArr2[] init       4
   kArr1   slicearray iArr, 0, 4
   kArr2   slicearray iArr, 5, 8
           PrtArr1k   kArr1
           PrtArr1k   kArr2
;string arrays
   SArr[]  fillarray  "a", "b", "c", "d", "e"
   SArr1[] init       3
   SArr2[] init       2
   SArr1   slicearray SArr, 0, 2
   SArr2   slicearray SArr, 3, 4
           PrtArr1S   SArr1
           PrtArr1S   SArr2
           turnoff
endin


instr Copyf2array
    prints "\n instr Copyf2array\n"
;i-rate
    iArr[]  init       8
    copyf2array iArr, giSine
    PrtArr1i iArr
;k-rate
    kArr[]  init       8
    copyf2array kArr, giSine
    PrtArr1k kArr
    turnoff
endin


instr Copya2ftab
    prints "\n instr Copya2ftab\n"
;i-rate
    iArr[]  fillarray  1, 2, 3, 4, 5, 6, 7, 8
    iFt1 ftgen 0, 0, lenarray(iArr), 2, 0
    copya2ftab iArr, iFt1
    prints "iFt1 values = "
    iIndx = 0
    until iIndx == lenarray(iArr) do
    printf_i "%.3f ", iIndx+1, table:i(iIndx, iFt1)
    iIndx += 1
    enduntil
    prints "\n"
;k-rate
    kArr[]  fillarray  1, 2, 3, 4, 5, 6, 7, 8
    iFt2 ftgen 0, 0, 8, 2, 0
    copya2ftab kArr, iFt2
    printks "iFt2 values = ", 0
    kIndx = 0
    until kIndx == lenarray(kArr) do
    printf "%.3f ", kIndx+1, table:k(kIndx, iFt2)
    kIndx += 1
    enduntil
    printks "\n", 0
    turnoff
endin


instr Arr_Num_Math
;+, -, *, / between an array and a number
    prints "\n instr Arr_Num_Math\n"
;i-rate
    iArr1[] fillarray 1, 2, 3
    iArr2[] = iArr1 + 10
    iArr3[] = iArr2 - 10
    iArr4[] = iArr3 * 10
    iArr5[] = iArr4 / 10
    prints "iArr1 = "
    PrtArr1i iArr1
    prints "iArr2 = iArr1 + 10: "
    PrtArr1i iArr2
    prints "iArr3 = iArr2 - 10: "
    PrtArr1i iArr3
    prints "iArr4 = iArr3 * 10: "
    PrtArr1i iArr4
    prints "iArr5 = iArr4 / 10: "
    PrtArr1i iArr5
;k-rate
    kArr1[] fillarray -1, -2, -3
    kArr2[] = kArr1 + 10
    kArr3[] = kArr2 - 10
    kArr4[] = kArr3 * 10
    kArr5[] = kArr4 / 10
    printks "kArr1 = ", 0
    PrtArr1k kArr1
    printks "kArr2 = kArr1 + 10: ", 0
    PrtArr1k kArr2
    printks "kArr3 = kArr2 - 10: ", 0
    PrtArr1k kArr3
    printks "kArr4 = kArr3 * 10: ", 0
    PrtArr1k kArr4
    printks "kArr5 = kArr4 / 10: ", 0
    PrtArr1k kArr5
    turnoff
endin


instr Arr_Arr_Math
; +, -, *, / between two arrays
    prints "\n instr Arr_Arr_Math\n"
;i-rate
    iArr1[] fillarray 1, 2, 3
    iArr2[] fillarray 4, 5, 6
    iArr3[] = iArr1 + iArr2
    iArr4[] = iArr1 - iArr2
    iArr5[] = iArr1 * iArr2
    iArr6[] = iArr1 / iArr2
    prints "iArr1 = "
    PrtArr1i iArr1
    prints "iArr2 = "
    PrtArr1i iArr2
    prints "iArr3[] = iArr1 + iArr2:\n        "
    PrtArr1i iArr3
    prints "iArr4[] = iArr1 - iArr2:\n        "
    PrtArr1i iArr4
    prints "iArr5[] = iArr1 * iArr2:\n        "
    PrtArr1i iArr5
    prints "iArr6[] = iArr1 / iArr2:\n        "
    PrtArr1i iArr6
;k-rate
    kArr1[] fillarray -1, -2, -3
    kArr2[] fillarray -4, -5, -6
    kArr3[] = kArr1 + kArr2
    kArr4[] = kArr1 - kArr2
    kArr5[] = kArr1 * kArr2
    kArr6[] = kArr1 / kArr2
    printks "kArr1 = ", 0
    PrtArr1k kArr1
    printks "kArr2 = ", 0
    PrtArr1k kArr2
    printks "kArr3[] = kArr1 + kArr2:\n        ", 0
    PrtArr1k kArr3
    printks "kArr4[] = kArr1 - kArr2:\n        ", 0
    PrtArr1k kArr4
    printks "kArr5[] = kArr1 * kArr2:\n        ", 0
    PrtArr1k kArr5
    printks "kArr6[] = kArr1 / kArr2:\n        ", 0
    PrtArr1k kArr6
    turnoff
endin


instr Minarray
    prints "\n instr Minarray\n"
;i-rate
    iArr[] fillarray -1, 0, 1.1, -3.3, 17
    iMin, iPos minarray iArr
    prints "iArr = "
    PrtArr1i iArr
    prints "iMin = value %.3f at index %d\n", iMin, iPos
;k-rate
    kArr[] fillarray -1, 0, 1.1, -3.3, 17
    kMin, kPos minarray kArr
    printks "kArr = ", 0
    PrtArr1k kArr
    printks "kMin = value %.3f at index %d\n", 0, kMin, kPos
    turnoff
endin


instr Maxarray
    prints "\n instr Maxarray\n"
;i-rate
    iArr[] fillarray -1, 0, 1.1, -3.3, 17
    iMax, iPos maxarray iArr
    prints "iArr = "
    PrtArr1i iArr
    prints "iMax = value %.3f at index %d\n", iMax, iPos
;k-rate
    kArr[] fillarray -1, 0, 1.1, -3.3, 17
    kMax, kPos maxarray kArr
    printks "kArr = ", 0
    PrtArr1k kArr
    printks "kMax = value %.3f at index %d\n", 0, kMax, kPos
    turnoff
endin


instr Sumarray
    prints "\n instr Sumarray\n"
;i-rate
    iArr[] fillarray -1, 0, 1.1, -3.3, 17
    iSum sumarray iArr
    prints "iArr = "
    PrtArr1i iArr
    prints "iSum = %.3f\n", iSum
;k-rate
    kArr[] fillarray -1, 0, 1.1, -3.3, 17
    kSum sumarray kArr
    printks "kArr = ", 0
    PrtArr1k kArr
    printks "kSum = %.3f\n", 0, kSum
    turnoff
endin


instr Scalearray
    prints "\n instr Scalearray\n"
;i-rate
    iArr[] fillarray -1, 0, 1.1, -3.3, 17
    prints "iArr before scaling: "
    PrtArr1i iArr
    scalearray iArr, 0, 10
    prints "iArr after scaling (0..10): "
    PrtArr1i iArr
;k-rate
    kArr[] fillarray -1, 0, 1.1, -3.3, 17
    printks "kArr before scaling: ", 0
    PrtArr1k kArr
    scalearray kArr, 0, 10
    printks "kArr after scaling (0..10): ", 0
    PrtArr1k kArr
    turnoff
endin


instr Maparray
    prints "\n instr Maparray\n"
;i-rate
    iArr1[] fillarray 1, 2, 3, 4, 5
    iArr2[] init 5
    iArr2 maparray iArr1, "sqrt"
    prints "iArr1 = "
    PrtArr1i iArr1
    prints "applied sqrt function to it = "
    PrtArr1i iArr2
;k-rate
    kArr1[] fillarray 1, 2, 3, 4, 5
    kArr2[] init 5
    kArr2 maparray kArr1, "sqrt"
    printks "kArr1 = ", 0
    PrtArr1k kArr1
    printks "applied sqrt function to it = ", 0
    PrtArr1k kArr2
    turnoff
endin


instr Assign
    prints "\n instr Assign\n"
;i-rate
    iArr1[] fillarray 1, 2, 3, 4, 5
    iArr2[] = iArr1
    PrtArr1i iArr2
;string array
    Sarr1[] fillarray "is", "this", "really", "possible", "?"
    ;not yet ...=)
;    Sarr2[] = Sarr1
;    PrtArr1S Sarr2 
;k-rate
    kArr1[] fillarray 1, 2, 3, 4, 5
    kArr2[] = kArr1
    PrtArr1k kArr2
    turnoff
endin


instr Genarray
    prints "\n instr Genarray\n"
;i-rate
    iArr1[] genarray   1, 5
    iArr2[] genarray   -1, 1, 0.5
    iArr3[] genarray   1, -1, -0.5
    iArr4[] genarray   -1, 1, 0.6
    PrtArr1i iArr1
    PrtArr1i iArr2
    PrtArr1i iArr3
    PrtArr1i iArr4
;k-rate
    kArr1[] genarray_i   1, 5
    kArr2[] genarray_i   -1, 1, 0.5
    kArr3[] genarray_i   1, -1, -0.5
    kArr4[] genarray_i   -1, 1, 0.6
    PrtArr1k kArr1
    PrtArr1k kArr2
    PrtArr1k kArr3
    PrtArr1k kArr4
    turnoff
endin

</CsInstruments>
<CsScore>
i "Fillarray" 0 .01
i "Lenarray" .01 .01
i "Slicearray" .02 .01
i "Copyf2array" .03 .01
i "Copya2ftab" .04 .01
i "Arr_Num_Math" .05 .01
i "Arr_Arr_Math" .06 .01
i "Minarray" .07 .01
i "Maxarray" .08 .01
i "Sumarray" .09 .01
i "Scalearray" .1 .01
i "Maparray" .11 .01
i "Assign" .12 .01
i "Genarray" .13 .01
</CsScore>
</CsoundSynthesizer>

