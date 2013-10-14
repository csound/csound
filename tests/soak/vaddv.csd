<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     -nm0 ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o cigoto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>


sr=44100
ksmps=128
nchnls=2

  opcode TableDumpSimp, 0, ijo
;prints the content of a table in a simple way
ifn, iprec, ippr   xin; function table, float precision while printing (default = 3), parameters per row (default = 10, maximum = 32)
iprec		=		(iprec == -1 ? 3 : iprec)
ippr		=		(ippr == 0 ? 10 : ippr)
iend		=		ftlen(ifn)
indx		=		0
Sformat	sprintf	"%%.%df\t", iprec
Sdump		=		""
loop:
ival		tab_i		indx, ifn
Snew		sprintf	Sformat, ival
Sdump		strcat		Sdump, Snew
indx		=		indx + 1
imod		=		indx % ippr
	if imod == 0 then
		puts		Sdump, 1
Sdump		=		""
	endif
	if indx < iend igoto loop
		puts		Sdump, 1
  endop

	instr 1
ifn1 = p4
ifn2 = p5
ielements = p6
idstoffset = p7
isrcoffset = p8
kval init 25
vaddv ifn1, ifn2, ielements, idstoffset, isrcoffset, 1
turnoff
	endin

	instr 2
TableDumpSimp p4, 3, 16
	endin

</CsInstruments>
<CsScore>

f 1 0 16 -7 1 15 16

f 2 0 16 -7 1 15 2


i2	0.0	0.2	1
i2	0.2	0.2	2
i1	0.4	0.01	1	2	5	3	8	
i2	0.8	0.2	1
i1	1.0	0.01	1	2	5	10	-2	
i2	1.2	0.2	1
i1	1.4	0.01	1	2	8	14	0	
i2	1.6	0.2	1
i1	1.8	0.01	1	2	8	0	14	
i2	2.0	0.2	1	
i1	2.2	0.002	1	1	8	5	2	
i2	2.4	0.2	1
e


</CsScore>
</CsoundSynthesizer>
