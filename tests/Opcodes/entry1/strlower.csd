<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n    ;;;no sound output
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions>
<CsInstruments>
;example of Joachim Heintz

  opcode FilSuf, S, So
  ;returns the suffix of a filename or path, optional in lower case 
Spath,ilow xin
ipos   strrindex Spath, "."	;look for the rightmost '.'
Suf    strsub    Spath, ipos+1	;extract the substring after "."
 if ilow != 0 then		;if ilow input is not 0 then 
Suf    strlower  Suf 		;convert to lower case
 endif
       xout      Suf
  endop

instr suff

ilow = p4
       prints    "Printing suffix:\n"
Suf    FilSuf    "/my/dir/my/file.WAV", ilow
       puts      Suf, 1

endin
</CsInstruments>
<CsScore>
i "suff" 0 1 0	;do not convert to lower case
i "suff" 3 1 1	;convert to lower case
e
</CsScore>
</CsoundSynthesizer>

