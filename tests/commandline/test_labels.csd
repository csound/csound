<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; Test opcode with label indented with tab
opcode test,0,0
	SKIP:
endop

; Test opcode with label indented with spaces
opcode test2,0,0
    SKIP2:
endop

; Test opcode with label with no indentation
opcode test3,0,0
SKIP3:
endop

; Test instrument with label indented with tab
instr 1
	Label1:
	print 1
	goto Label1Done
	Label1Done:
endin

; Test instrument with label indented with spaces
instr 2
    Label2:
    print 2
    goto Label2Done
    Label2Done:
endin

; Test instrument with mixed indentation (tabs and spaces)
instr 3
		TabLabel:
        SpaceLabel:
	print 3
endin

</CsInstruments>
<CsScore>

i 1 0 0.01
i 2 0 0.01
i 3 0 0.01
e
</CsScore>
</CsoundSynthesizer>
