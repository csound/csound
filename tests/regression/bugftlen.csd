<CsoundSynthesizer>

<CsInstruments>
instr 1
 /* size = 11: ftlen reports length 10, index 10 returns 10 (instead of 11) */
 ift1 ftgen 0, 0, 11, -2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
 print ftlen(ift1)
 print table:i(10,ift1)
 /* size = -11: ftlen reports length 11, index 10 returns 11 */
 ift2 ftgen 0, 0, -11, -2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
 print ftlen(ift2)
 print table:i(10,ift2)
endin


</CsInstruments>

<CsScore>
i1 0 0
e
</CsScore>

</CsoundSynthesizer>
