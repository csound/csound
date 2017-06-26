<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
; cyclic bells
; author: steven yi
; released: 2004.02.03
;
; I had gone to East-West books in New York and while there had heard some lovely bell sounds.  
; The sounds were coming from a thing in the back that had water flowing into a small basin.  
; Inside the basin were a ring of fixed, different sized bells, as well as a couple of floating bells.
; As the floating bells floated around the circle they would occasionally hit the ring of bells.
;
; This version is based on score generation functions I had written in python, and was written
; with Csound facilities alone.
;
; This is meant to be performed in realtime.  The commandline I used to operate this is:
;
; csound -d -o dac cyclicBells.csd

nchnls  =   2

gi_hit_probability	= 0.20

	instr 1	;fmbell
ipch 	= p4
ipch2	= p5

ipch 	= (ipch < 15 ? cpspch(ipch) : ipch);
ipch2 	= (ipch2 < 15 ? cpspch(ipch2) : ipch2);

kpchline 	line ipch, p3, ipch2

iamp 	= ampdb(p6)
iSpace	= p7
ienvType	= p8

itable	= 4

kenv 	init 	0

print	ipch, ipch2

if ienvType == 0 kgoto env0  ; adsr
if ienvType == 1 kgoto env1  ; pyramid
if ienvType == 2 kgoto env2  ; ramp

env0:

	kenv	adsr	.3, .2, .9, .5

	kgoto endEnvelope

env1:
	
	kenv 	linseg	0, p3 * .5, 1, p3 * .5, 0

	kgoto endEnvelope

env2:
	
	kenv	linseg 	0, p3 - .1, 1, .1, 0	

	kgoto endEnvelope

endEnvelope:

; INSERT SOUND GENERATING CODE HERE 

kc1 = 5
kc2 = 5
kvdepth = 0.005
kvrate = 6
ifn1 = 1
ifn2 = 1
ifn3 = 1
ifn4 = 1
ivfn = 1


aout	fmbell	iamp, kpchline, kc1, kc2, kvdepth, kvrate, ifn1, ifn2, ifn3, ifn4, ivfn



; PANNING CODE
iSpace 	= (iSpace-.5) * 3.14159265359
krtl     	= sqrt(2) / 2 * (cos(iSpace) + sin(iSpace)) ; CONSTANT POWER PANNING
krtr     	= sqrt(2) / 2 * (cos(iSpace) - sin(iSpace))	; FROM C.ROADS "CM TUTORIAL" pp460

aLeft 	=	aout * krtl
aRight	=	aout * krtr

	outs aLeft, aRight

;ga1 = ga1 + aLeft
;ga2 = ga2 + aRight 


	endin

	instr 2	;event generator
iTableNum	= p4
iBaseAmp 	= p5
iPan	= p6

kcount 	init 5 * kr
kdur	init 4 * kr
kHit 	init 0

kIndex	init 0

if (kcount < kdur) kgoto counterup
	
	kHit rnd31 .5, 0
	kHit = kHit + .5

	if (kHit > gi_hit_probability) kgoto nohit
		kpch1 	table 	0, iTableNum
		kpch2	table	kIndex, iTableNum
		
		kamp	= iBaseAmp - (kHit * 5)

		event "i", 1, 0, 10, kpch1, kpch1, kamp, iPan, 0
		event "i", 1, 0, 10, kpch2, kpch2, kamp, iPan, 0

		kdur = (kHit * 3) + 3
		kdur = kdur * kr


nohit:
		kIndex 	= kIndex + 1

		if (kIndex < 5) kgoto noZeroing
			kIndex = 0

noZeroing:

	kcount = 0
counterup:
	kcount = kcount + 1

	endin


</CsInstruments>
<CsScore>

f1 0 65536 10 1

;center pitches = [10.00, 10.01, 10.02, 10.04, 10.07]
;left pitches = [10.07, 10.00, 10.01, 10.02, 10.04]
;right pitches = [10.01, 10.02, 10.04, 10.07, 10.00]

f10 0 8 -2 10.00 10.01 10.02 10.04 10.07 0 0 0
f11 0 8 -2 10.07 10.00 10.01 10.02 10.04 0 0 0
f12 0 8 -2 10.01 10.02 10.04 10.07 10.00 0 0 0



i2 0 3600 10 62 0
i2 .33 3600 11 64 -.2
i2 .49 3600 12 64 .2



i1	0.0	6.0	10.00	10.00	70	0	0	
e

</CsScore>

</CsoundSynthesizer>
