; ptofI
; ptofK
; ptofA
; ftopI
; ftopK
; ftopA
; envAr
; envKr
; envAcont
; envKrFilt
; envKrModFilt
; mtofI
; mtofK
; mtofA
; svf
; metaFilt
; porta
; syncLFOa
; syncLFOk
; bal
; limitPolyphony

#define minCPS # 30 #
#define maxCPS # 9000 #

#define MIDI_CPS_MAX # 12543.85 #
#define MIDI_CPS_MIN # 8.176 #

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

opcode ptofI, i, iii
    iIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    iIn limit iIn, 0, 1
    iOut = iMin * (iMaxOverMin ^ iIn)
    xout iOut
endop

opcode ptofK, k, kii
    kIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    xout kOut
endop

opcode ptofA, a, aii
    setksmps 1
    aIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn downsamp aIn
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    aOut upsamp kOut
    xout aOut
endop

opcode ftopI, i, iii
    iIn, iMin, iMax xin
	iOneDivMin = 1 / iMin
	iOneDivLogMaxOverMin = 1 / log(iMax / iMin)
	iOut = log(iIn * iOneDivMin) * iOneDivLogMaxOverMin
    xout iOut
endop

opcode ftopK, k, kii
    kIn, iMin, iMax xin
	iOneDivMin = 1 / iMin
	iOneDivLogMaxOverMin = 1 / log(iMax / iMin)
	kOut = log(kIn * iOneDivMin) * iOneDivLogMaxOverMin
    xout kOut
endop

opcode ftopA, a, aii
    setksmps 1
    aIn, iMin, iMax xin
    iOneDivMin = 1 / iMin
	iOneDivLogMaxOverMin = 1 / log(iMax / iMin)
	kIn downsamp aIn
	kOut = log(kIn * iOneDivMin) * iOneDivLogMaxOverMin
	aOut upsamp kOut
    xout aOut
endop

opcode envAr, a, kiiii
	kType, iAtk, iDec, iSus, iRel xin
	iAtk = iAtk + .001
	iDec = iDec + .001
	iSus = iSus + .001
	iRel = iRel + .001
	if kType == 0 then
		aOut expsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	elseif kType == 1 then
		aOut linsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	endif
	xout aOut
endop

opcode envKr, k, kiiii
	kType, iAtk, iDec, iSus, iRel xin
	iAtk = iAtk + .001
	iDec = iDec + .001
	iSus = iSus + .001
	iRel = iRel + .001
	if kType == 0 then
		kOut expsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	elseif kType == 1 then
		kOut linsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	endif
	xout kOut
endop

; An audio rate envelope generator with start value.
opcode envAcont, a, iiiiik
	iStart, iAtk, iDec, iSus, iRel, kType xin
	iAtk = iAtk + .001
	iDec = iDec + .001
	iSus = iSus + .001
	iRel = iRel + .001
	if kType == 0 then
		aOut expseg iStart, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	elseif kType == 1 then
		aOut linseg iStart, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	endif
	xout aOut
endop

opcode envKrFilt, k, kiiiiiii
	kType, iAtk, iDec, iSus, iRel, iCPS, iFBas, iFRng xin
	iAtk = iAtk + .001
	iDec = iDec + .001
	iSus = iSus + .001
	iRel = iRel + .001
	if kType == 0 then
		kOut expsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	elseif kType == 1 then
		kOut linsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	endif

	iPitch ftopI iCPS, $minCPS, $maxCPS
	iPitch_plus_iFBas = iPitch + iFBas
	kOut = (kOut * iFRng) + iPitch_plus_iFBas
	kOut limit kOut, 0, .9999
	kOut ptofK kOut, $minCPS, $maxCPS

	xout kOut
endop

opcode envKrModFilt, k, kkiiiiiii
	kType, kMod, iAtk, iDec, iSus, iRel, iCPS, iFBas, iFRng xin
	iAtk = iAtk + .001
	iDec = iDec + .001
	iSus = iSus + .001
	iRel = iRel + .001
	if kType == 0 then
		kOut expsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	elseif kType == 1 then
		kOut linsegr .001, iAtk, 1, iDec, (iSus * .999) + .001, iRel, .001
	endif

	iPitch ftopI iCPS, $minCPS, $maxCPS
	iPitch_plus_iFBas = iPitch + iFBas
	kOut = kOut + kMod
	kOut = (kOut * iFRng) + iPitch_plus_iFBas
	kOut limit kOut, 0, .9999
	kOut ptofK kOut, $minCPS, $maxCPS

	xout kOut
endop

opcode mtofI, i, i
	iIn xin
	iOut = (440.0*exp(log(2.0)*(iIn-69.0)/12.0))
	xout iOut
endop

opcode mtofK, k, k
	kIn xin
	kOut = (440.0*exp(log(2.0)*(kIn-69.0)/12.0))
	xout kOut
endop

opcode mtofA, a, a
	aIn xin
	aOut = (440.0*exp(log(2.0)*(aIn-69.0)/12.0))
	xout aOut
endop

opcode svf, a, akkk
	aIn, kFrq, kQ, kType xin
	denorm aIn
	kQ limit kQ, 1, 500
	aLP, aHP, aBP svfilter aIn, kFrq, kQ
	if kType == 0 then
		aOut = aLP
	elseif kType == 1 then
		aOut = aHP
	elseif kType == 2 then
		aOut = aBP
	endif
	xout aOut
endop

opcode metaFilt, a, akkkkkk
	aIn, kCutoff, kQ, kType, kAlgo, kDist, kAsym xin
	kQ limit kQ, 0, 2
	if kAlgo == 0 then
		denorm aIn
		aOut moogladder aIn, kCutoff, kQ, 1
	elseif kAlgo == 1 then
		denorm aIn
		aOut lpf18 aIn, kCutoff, kQ, kDist
	elseif kAlgo == 2 then
		denorm aIn
		aOut lowpass2 aIn, kCutoff, kQ * 20
		;aOut = aOut * .5
	elseif kAlgo == 3 then		
		denorm aIn
		kCutoff limit kCutoff, 240, 10000
		aOut tbvcf aIn, kCutoff, kQ * 2, kDist, kAsym
	elseif kAlgo == 4 then		
		aOut svf aIn, kCutoff, kQ * 30, kType
		;aOut = aOut * .25	
	endif

	aOut dcblock aOut
	xout aOut
endop

opcode porta, a, aki
	setksmps 1
	aIn, khtim, isig xin
	denorm aIn
	kIn downsamp aIn
	kOut portk kIn, khtim, isig
	aOut upsamp kOut
	xout aOut
endop

opcode syncLFOa, a, kkkakk
	kAmp, kFrq, kType, aSync, kPhase, kSyncEnabled xin
	if kType > 10 then 
		kType = 10
	endif
	if kSyncEnabled < .0001 then
		aSync = 0
	endif
	aLFO oscilikts kAmp, kFrq, kType + $syncLFO_TableBase, aSync, kPhase
	xout aLFO
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

opcode bal, a, aa
	setksmps 1
	aIn, aComp xin
	kRMS rms aComp
	aOut gain aIn, kRMS 
	xout aOut
endop

opcode limitPolyphony, k, ki
	kLimit, instrNum xin
	kInstrNum init instrNum
	kActive active kInstrNum
	if kActive > kLimit then
		turnoff2 kInstrNum, 1, 0
		kInstances = kLimit
	else
		kInstances = kActive
	endif
	
	xout kInstances
endop