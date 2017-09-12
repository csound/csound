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

;an 'empty' function table with 10 points
giTable ftgen   0, 0, -10, 2, 0


  instr 1

;print inital values of giTable
        puts    "\nInitial table content:", 1
indx    =       0
  until indx == ftlen(giTable) do
iVal    table   indx, giTable
        printf_i "Table index %d = %f\n", 1, indx, iVal
indx += 1
  od

;create array
kArr[]  init    10

;fill in values
kArr    genarray 1, 10

;print array values
        printf  "%s", 1, "\nArray content:\n"
kndx    =       0
  until kndx == lenarray(kArr) do
        printf  "kArr[%d] = %f\n", kndx+1, kndx, kArr[kndx]
kndx    +=      1
  od

;copy array values to table
        copya2ftab kArr, giTable

;print modified values of giTable
        printf  "%s", 1, "\nModified table content after copya2ftab:\n"
kndx    =       0
  until kndx == ftlen(giTable) do
kVal    table   kndx, giTable
        printf  "Table index %d = %f\n", kndx+1, kndx, kVal
kndx += 1
  od

;turn instrument off
        turnoff
  endin
  
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
