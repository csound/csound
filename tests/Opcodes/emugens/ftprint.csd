<CsoundSynthesizer>
<CsOptions>

--nosound

</CsOptions>
<CsInstruments>

; This is the example file for ftprint

/*

  ftprint

  Print the contents of an f-table
  (mostly for debuggin purposes)

  ftprint ifn, ktrig=1, kstart=0, kend=0, kstep=1, inumcols=0

  ifn: the table to print
  ktrig: table will be printed whenever this changes
         from non-positive to positive
  kstart: start index
  kend: end index (non inclusive)
  kstep: number of elements to skip
  inumcols: number of elements to print per line

  See also: printarray

*/

instr 1
  ifn   ftgentmp 0, 0, -13, -2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12

  ; print all elements
  ftprint ifn

  ; print all elements in columns of 4 elements
  ftprint ifn, 1, 0, 0, 1, 4

  ; assume that a table holds a 2D matrix, print the matrix 
  imatrix  ftgentmp 0, 0, 0, -2, \
      00, 01, 02, 03, 04,  \
      10, 11, 12, 13, 14,  \
      20, 21, 22, 23, 24,  \
      30, 31, 32, 33, 34

  ; print the whole matrix, 5 columns per line
  ftprint imatrix, 1, 0, 0, 1, 5

  ; print one row
  irow = 2
  inumcols = 5
  ftprint imatrix, 1, 2*inumcols, 3*inumcols

  ; print one column
  ftprint imatrix, 1, 3, 0, inumcols, 1

  turnoff
endin

</CsInstruments>

<CsScore>
i 1 0 0.01
</CsScore>

</CsoundSynthesizer>
