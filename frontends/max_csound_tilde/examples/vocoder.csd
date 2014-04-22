<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr      =  44100
ksmps   =  32
nchnls  =  2
0dbfs   =  1

giSine ftgen 1, 0, 16384, 10, 1 ; Generate a sine wave table.

#define syncLFO_TableBase  # 150 #
#define syncLFO_TableRange # 11 #
giLFOSine			 ftgen 150, 0, 4096, 10, 1
giLFOTriangleBi      ftgen 151, 0, 4096, 7, 0, 1024, 1, 1024, 0, 1024, -1, 1024, 0
giLFOSquareBi		 ftgen 152, 0, 4096, 7, 1, 2048, 1, 0, -1, 2048, -1
giLFOSquareUni		 ftgen 153, 0, 4096, 7, 1, 2048, 1, 0, 0, 2048, 0
giLFOSquareUniNeg	 ftgen 154, 0, 4096, 7, 0, 2048, 0, 0, -1, 2048, -1
giLFOUpBi	         ftgen 155, 0, 4096, 7, -1, 4096, 1
giLFODownBi	         ftgen 156, 0, 4096, 7, 1, 4096, -1
giLFOUpUni	         ftgen 157, 0, 4096, 7, 0, 4096, 1
giLFODownUni	     ftgen 158, 0, 4096, 7, 1, 4096, 0
giLFOUpUniNeg        ftgen 159, 0, 4096, 7, 0, 4096, 1
giLFODownUniNeg	     ftgen 160, 0, 4096, 7, 1, 4096, 0
 
opcode mtofK, k, k
	kIn xin
	kOut = (440.0*exp(log(2.0)*(kIn-69.0)/12.0))
	xout kOut
endop

opcode ptofK, k, kii
    kIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    xout kOut
endop

opcode syncLFOk, k, kkkkkk
	kAmp, kFrq, kType, kSync, kPhase, kSyncEnabled xin
	if kType > 10 then 
		kType = 10
	endif
	aSync upsamp kSync
	if kSyncEnabled < .0001 then
		aSync = 0
	else
		aSync upsamp kSync
	endif
	aLFO oscilikts kAmp, kFrq, kType + $syncLFO_TableBase, aSync, kPhase
	kLFO downsamp aLFO
	xout kLFO
endop

/*
SYNTAX
asig Vocoder aexc, ain, kminf, kmaxf, kq, ibands

INITIALIZATION
ibands - number of filter bands between kminf and kmaxf

PERFORMANCE
asig - output
aexc - excitation signal, generally a broadband source (ie. lots of spectral components)
ain - input signal, generally a signal with a strong spectral envelope or contour, formants, etc. (such as vocal sound)
kminf - lowest analysis frequency
kmaxf - highest analysis frequency
kq - filter Q 
*/

opcode Vocoder, a, aakkkpp

	as1,as2,kmin,kmax,kq,ibnd,icnt  xin

	if kmax < kmin then
		ktmp = kmin
		kmin = kmax
		kmax = ktmp
	endif

	if kmin == 0 then 
		kmin = 1
	endif

	if (icnt >= ibnd) goto bank
	abnd   Vocoder as1,as2,kmin,kmax,kq,ibnd,icnt+1

	bank:
	kfreq = kmin*(kmax/kmin)^((icnt-1)/(ibnd-1))
	;kfreq tab icnt - 1, giFrqTable
	kbw = kfreq/kq
	an  butterbp  as2, kfreq, kbw
	an  butterbp  an, kfreq, kbw
	as  butterbp  as1, kfreq, kbw
	as  butterbp  as, kfreq, kbw
	ao balance as, an

	amix = ao + abnd
    xout amix
endop


instr 1
	aL inch 1
	aR inch 2

	iBands	chnget "bands"
	kQ		chnget "Q"
	kFund	chnget "fund"
	kFrqMin chnget "frqMin"
	kFrqMax chnget "frqMax"
	kVcoTyp chnget "vcoTyp"
	
	kNote0 chnget "n0"
	kNote1 chnget "n1"
	kNote2 chnget "n2"
	kNote3 chnget "n3"
	kTrans chnget "trans"

	kLFOamp chnget "lfoAmp"
	kLFOfrq chnget "lfoFrq"
	kLFOtyp chnget "lfoTyp"

	kDryWet chnget "drywet" ; [-1,1]
	kDryWet = (kDryWet + 1) * .5

	kQ portk kQ, .005

	kFrqMin ptofK kFrqMin, 50, 9000
	kFrqMax ptofK kFrqMax, 50, 9000
	kFrqMin portk kFrqMin, .005
	kFrqMax portk kFrqMax, .005

	
	kLFO syncLFOk kLFOamp, kLFOfrq, kLFOtyp, 0, 0, 0
	kLFO portk kLFO, .0005

	kNote0 = kNote0 + kTrans + kLFO
	kNote1 = kNote1 + kTrans + kLFO
	kNote2 = kNote2 + kTrans + kLFO
	kNote3 = kNote3 + kTrans + kLFO

	kNote0 limit kNote0, 0, 127
	kNote1 limit kNote1, 0, 127
	kNote2 limit kNote2, 0, 127
	kNote3 limit kNote3, 0, 127

	kFrq0 mtofK kNote0
	kFrq1 mtofK kNote1
	kFrq2 mtofK kNote2
	kFrq3 mtofK kNote3
	
	if kVcoTyp < 1 then ; saw
		aExcite0 vco2 .25, kFrq0, 0
		aExcite1 vco2 .25, kFrq1, 0
		aExcite2 vco2 .25, kFrq2, 0
		aExcite3 vco2 .25, kFrq3, 0
	elseif kVcoTyp < 2 then	; square
		aExcite0 vco2 .25, kFrq0, 10
		aExcite1 vco2 .25, kFrq1, 10
		aExcite2 vco2 .25, kFrq2, 10
		aExcite3 vco2 .25, kFrq3, 10	
	elseif kVcoTyp < 3 then ; triangle
		aExcite0 vco2 .25, kFrq0, 12
		aExcite1 vco2 .25, kFrq1, 12
		aExcite2 vco2 .25, kFrq2, 12
		aExcite3 vco2 .25, kFrq3, 12
	endif

	anoi = aExcite0 + aExcite1 + aExcite2 + aExcite3
	aOut Vocoder anoi, aL, kFrqMin, kFrqMax, kQ, iBands

	kCorrectionAmp = 1 + (kQ / 100) * 3
	aOut = aOut * kCorrectionAmp
	aOut = (aOut * kDryWet) + (aL * (1 - kDryWet))

	outs aOut, aOut
endin

</CsInstruments>
<CsScore>
f0 86400
i 1 0 86400
e
</CsScore>
</CsoundSynthesizer>
