<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o cigoto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr=44100
ksmps=128
nchnls=2

	instr 1
ifn1 = p4
ival = p5
ielements = p6
idstoffset = p7
kval init 25
vpow ifn1, ival, ielements, idstoffset, 1
	endin

	instr 2	;Printtable
itable = p4
isize = ftlen(itable)
kcount init 0
kval table kcount, itable
printk2 kval

if (kcount == isize) then
  turnoff
endif

kcount = kcount + 1 
	endin


</CsInstruments>

<CsScore>

f 1 0 16 -7 1 16 17

i2	0.0	0.2	1
i1	0.4	0.01	1	2	3	4
i2	0.8	0.2	1
i1	1.0	0.01	1	0.5	5	-3
i2	1.2	0.2	1
i1	1.4	0.01	1	1.5	10	12
i2	1.6	0.2	1
e

</CsScore>

</CsoundSynthesizer>