<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; Example file ftslice

/*

ftslice: 

Copy slice from source table to destination table

Syntax:

ftslice ifnsource, ifndest, kstart=0, kend=0, kstep=1

ifnsource: source table
ifndest: destination table
kstart: the index to start copying from
kend: the end index to stop copying. This is NOT inclusive. 0=end of table
kstep: how many elements to skip

See also: tablecopy, tableicopy, tab2array
*/

instr 1
  ifn   ftgentmp 0, 0, -13, -2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
  idest ftgentmp 0, 0, -11, -2,  0   ; empty table of size 11

  ; copy only even elements
  ftslice ifn, idest, 0, 0, 2
  ftprint idest

  ; copy too many elements - only the elements which fit in the dest table
  ; are copyed

  ftslice ifn, idest
  ftprint idest  

  turnoff
endin


</CsInstruments>
<CsScore>
i 1 0   0.1

</CsScore>
</CsoundSynthesizer>
