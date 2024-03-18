<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

;browse for text files in current directory
instr 1
iCnt init 0
SFilenames[] directory ".", ".wav"
iNumberOfFiles lenarray SFilenames

until iCnt>=iNumberOfFiles do
	printf_i "Filename = %s \n", 1, SFilenames[iCnt]
	iCnt = iCnt+1
od
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
