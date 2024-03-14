<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out and realtime midi in
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

giengine fluidEngine
isfnum	 fluidLoad "sf_GMbank.sf2", giengine, 1

instr 1
iCnt init 0
SSoundFontPrograms[] fluidInfo giengine
iNumberOfPrograms lenarray SSoundFontPrograms

until iCnt>=iNumberOfPrograms do
	printf_i "%s\n", 1, SSoundFontPrograms[iCnt]
	iCnt = iCnt+1
od
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
