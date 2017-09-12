<CsoundSynthesizer>
<CsInstruments>
/*
testing long strings
looks like this is the reason for the bug
*/
instr testimonium ;no
endin
instr testimoniu ;no
endin
instr testimoni ;no
endin
instr testimon ;no
endin
instr testimo ;ok
endin
</CsInstruments>
<CsScore>
i "testimonium" 0 1 
i "testimoniu" 0 1 
i "testimoni" 0 1 
i "testimon" 0 1 
i "testimo" 0 1 
</CsScore>
</CsoundSynthesizer>
