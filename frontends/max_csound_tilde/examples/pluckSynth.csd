<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 32
nchnls  = 2
0dbfs   = 1

massign 0,0
massign 1,10

; Each instance of instr #10 gets an ID = { 0 .. gkMaxIDs-1 }

gkLimit  init 1 ; Initial polypohny limit.
gkMaxIDs init 2 ; gkMaxIDs-1 = the maximum ID.  gkLimit must be < gkMaxIDs.
gkCurID  init 1 ; 0 == the minimum ID

#define minCPS	# 30 #
#define maxCPS	# 9000 #

; frequency to normalized [0-1] pitch
opcode ftopI, i, iii
    iIn, iMin, iMax xin
	iOneDivMin = 1 / iMin
	iOneDivLogMaxOverMin = 1 / log(iMax / iMin)
	iOut = log(iIn * iOneDivMin) * iOneDivLogMaxOverMin
    xout iOut
endop

; normalized [0-1] pitch to frequency
opcode ptofK, k, kii
    kIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    xout kOut
endop

; a-rate MIDI envelope
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

; k-rate MIDI envelope
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

; svfilter wrapper
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

; instr #1 will set gkLimit and gkMaxIDs from the "poly" channel.
; If the value of gkLimit changes, all instr #10 instances will be turned off.  
; This is always true:  gkMaxIDs = gkLimit + 1

instr 1
	kNewLimit chnget "poly"
	kNewLimit = round(kNewLimit)
	if kNewLimit != gkLimit && kNewLimit >= 1 then
		gkLimit = kNewLimit
	    gkMaxIDs = gkLimit + 1
		gkCurID = gkLimit
		turnoff2 10, 0, 1 ; turn off all instances of instr #10
	endif
endin

instr 10
	kON init 1
	kOnce init 1
	if kOnce == 1 then
		kOnce = 0                          ; Make sure this section is run only once per instance.
		gkCurID = (gkCurID + 1) % gkMaxIDs ; Increment the current ID (given to the next instance).
		kID = gkCurID                      ; Assign current ID to current instance.
	endif

	; If iID is the oldest ID, then set kON = 0 so that this instance will
	; turn itself off (after a ramp down of amplitude).
	if kON == 1 && kID == ((gkCurID + gkMaxIDs - gkLimit) % gkMaxIDs) then
		kON = 0
	endif

	; Generate an envelope that controls when turnoff is called.
	kONenv port kON, .05, 1	

	if kONenv < .001 then
	    turnoff
	else
	;{
		icps  cpsmidi
		ivel  veloc 0, .6
		ifn   init 0

		iamp  = (ivel ^ 2) * .78 + .22 

		imeth chnget "meth"
		imeth = imeth + 1

		ip1   chnget "p1"
		ip2   chnget "p2"
 
		iAtk  chnget "a"
		iDec  chnget "d"
		iSus  chnget "s"
		iRel  chnget "r"

		iAtk  = (iAtk ^ 2)
		iDec  = (iDec ^ 2)
		iSus  = iSus ^ 2
		iRel  = (iRel ^ 2)

		iFAtk chnget "fa"
		iFDec chnget "fd"
		iFSus chnget "fs"
		iFRel chnget "fr"
		iFBas chnget "fbas"
		iFRng chnget "frng"

		iFAtk = (iFAtk ^ 2)
		iFDec = (iFDec ^ 2)
		iFSus = iFSus ^ 2
		iFRel = (iFRel ^ 2)
		iFRng = iFRng * (ivel ^ 2)	

		kcps  chnget "cps"
		kcps  = kcps * icps	
		kQ    chnget "Q"
		kType chnget "type"
	
		aPluck   pluck iamp, kcps, icps, ifn, imeth, ip1, ip2
		ampEnv   envAr 0, iAtk, iDec, iSus, iRel	
		kFiltEnv envKrFilt 0, iFAtk, iFDec, iFSus, iFRel, icps, iFBas, iFRng
		aFilt    svf aPluck, kFiltEnv, kQ, kType
		aOut     = aFilt * ampEnv * kONenv
		         outs aOut, aOut
	;}	         
	endif
endin

</CsInstruments>
<CsScore>
f0 86400
i1 0 -1
e
</CsScore>
</CsoundSynthesizer>
