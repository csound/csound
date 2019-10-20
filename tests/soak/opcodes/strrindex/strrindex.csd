<CsoundSynthesizer>
<CsInstruments>
;example by Joachim Heintz

  opcode FilNam, S, S
;returns the name of a file path
Spath  xin
ipos   strrindex Spath, "/"	;look for the rightmost '/'
Snam   strsub    Spath, ipos+1	;extract the substring 
       xout      Snam
  endop
  
instr name
       prints    "Printing name:\n"
Snam   FilNam    "/my/dir/my/file.WAV"
       puts      Snam, 1  

endin 
</CsInstruments>
<CsScore>
i "name" 0 0
e
</CsScore>
</CsoundSynthesizer>

