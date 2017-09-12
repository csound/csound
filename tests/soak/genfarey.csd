<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr=44100
ksmps=10
nchnls=1

instr 4
      kndx init 0 ; read out elements of F_8 one by one and print to file
      if (kndx < 23) then    
      	 kelem tab kndx, 1
      	 fprintks "farey8table.txt", "%2.6f\\n", kelem
      	 kndx = kndx+1
      endif
endin
</CsInstruments>
<CsScore>
; initialise integer for Farey Sequence F_8
f1 0 -23 "farey" 8 0
      ; if mode=0 then the table stores all elements of the Farey Sequence
      ; as fractions in the range [0,1]
i4	0     1
e
</CsScore>
</CsoundSynthesizer>
