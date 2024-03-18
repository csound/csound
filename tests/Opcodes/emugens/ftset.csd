csdo<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; Example file ftset

/*

ftset: 

Write a value to the whole table, or a slice of it.

Syntax:

ftset ktablenum, kvalue, kstart=0, kend=0, kstep=1
ftset itablenum, ivalue, istart=0, iend=0, istep=1

ktablenum / itablenum: 
table to be modified

kvalue / ivalue: 
value to write to the table

kstart / istart: 
    the index to start modifying

kend / iend: 
   the end index to stop modifying. This is NOT inclusive. 0=end of table
   Any negative index will be interpreted as counting from the end of the 
   table, so -2 will modify the whole table but the last two elements 

kstep / istep: 
    how many elements to skip

See also: tablecopy, tableicopy, tab2array
*/

instr 1
  ; clear the table
  itable ftgentmp 0, 0, 13, -2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
  ftset itable, 0
  ; print it at perf-time
  ftprint itable, -1
  turnoff
endin

instr 2
  ; Set all elements but the last 5 to 99
  itable ftgentmp 0, 0, 13, -2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12

  ftset itable, 99, 0, -5
  ftprint itable, -1
  turnoff
endin

instr 3
  ; ftset works at k-time so it can be used inside an if clause
  ; Set all even elements to 0 at the 10th k-cycle
  itable ftgentmp 0, 0, 10, -2,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9
  kcycle = timeinstk()
  if kcycle == 10 then
    ftset itable, 0, 0, 0, 2
  endif
  println "cycle num: %d", kcycle
  ftprint itable, -1  ; print at each cycle
  ; turnoff the instrument if table was indeed modified
  if tab:k(6, itable) == 0 then
    println "Table was modified, turning off"
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0   0.1
i 2 +   0.1
i 3 +   0.1

</CsScore>
</CsoundSynthesizer>
