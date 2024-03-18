<CsoundSynthesizer>
<CsOptions>
-odac       ;   
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32

; by tgrey 2020
instr 1

iLoopStart = p4
iLoopEnd   = p5
iOffset    = p6

iCount init iLoopStart

	
if(iLoopStart<iLoopEnd) then		; loop going up
	while iCount <= iLoopEnd do
		iVal = iCount+iOffset
		iRound = round(iVal)
		iInt = int(iVal)
		iFloor = floor(iVal)
		iCeil = ceil(iVal)
		print iVal, iRound, iInt, iFloor, iCeil
		iCount = iCount + 1		
	od
	
elseif(iLoopEnd<iLoopStart) then	; loop going down
	while iCount >= iLoopEnd do
		iVal = iCount+iOffset
		iRound = round(iVal)
		iInt = int(iVal)
		iFloor = floor(iVal)
		iCeil = ceil(iVal)
		print iVal, iRound, iInt, iFloor, iCeil
		iCount = iCount - 1		
	od
endif
endin
</CsInstruments>
<CsScore>
i1 0 .1 0 10 .5
i1 .2 .1 0 -10 .5
e
</CsScore>
</CsoundSynthesizer>
