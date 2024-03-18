<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pcount.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; This UDO returns a pfield value but returns 0 if it does not exist.
opcode  mypvalue, i, i
	index  xin
	inum   pcount
	if	  (index > inum)  then
		iout = 0.0
	else
		iout pindex index
	endif
	
	xout	iout
endop
	
; Envelope UDO that reads parameters from a flexible number of pfields
; Syntax:   kenv  flexlinseg  ipstart
;           ipstart is the first pfield of the envelope
;               parameters.  Reads remaining pfields (up to 21 of them).
;           kenv is the output envelope.

opcode  flexlinseg, k, i
	ipstart xin
	
	iep1   mypvalue	ipstart
	iep2   mypvalue	ipstart + 1
	iep3   mypvalue	ipstart + 2
	iep4   mypvalue	ipstart + 3
	iep5   mypvalue	ipstart + 4
	iep6   mypvalue	ipstart + 5
	iep7   mypvalue	ipstart + 6
	iep8   mypvalue	ipstart + 7
	iep9   mypvalue	ipstart + 8
	iepa   mypvalue	ipstart + 9
	iepb   mypvalue	ipstart + 10
	iepc   mypvalue	ipstart + 11
	iepd   mypvalue	ipstart + 12
	iepe   mypvalue	ipstart + 13
	iepf   mypvalue	ipstart + 14
	iepg   mypvalue	ipstart + 15
	ieph   mypvalue	ipstart + 16
	iepi   mypvalue	ipstart + 17
	iepj   mypvalue	ipstart + 18
	iepk   mypvalue	ipstart + 19
	iepl   mypvalue	ipstart + 20

	kenv   linseg	 iep1, iep2, iep3, iep4, iep5, iep6, iep7, iep8, \
	                   iep9, iepa, iepb, iepc, iepd, iepe, iepf, iepg, \
	                   ieph, iepi, iepj, iepk, iepl	
	xout   kenv
endop
	
instr 1
; This instrument only requires 3 pfields but can accept up to 24.
; (You will still get warnings about more than 3).	
kenv flexlinseg  4		; envelope params start at p4
aout oscili kenv*.5, 256, 1
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f1  0 8192 10 1

i1  0 2  0.0 1.0 1.0 1.0 0.0
i1  2 2  0.0 0.1 1.0 1.0 1.0 0.1 0.0
i1  4 2  0.0 0.5 0.0	  			; one problem is that "missing" pfields carry
i1  6 2  0.0 0.5 0.0 !	  			; now we can fix this problem with !
i1  8 10  0.0  3.0 1.0  0.3 0.1  0.3 1.0  0.3 0.1  0.3 1.0  0.3 0.1  0.8 0.9  5.0 0.0

e
</CsScore>
</CsoundSynthesizer>
