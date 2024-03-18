<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o diskin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
0dbfs = 1

;load all samples in a given directory into function tables and play them using instrument 1000
instr 1
   iFirstTableNumber = 60;
   iFileCount init 1
   iNumberOfFiles ftsamplebank "../examples/", iFirstTableNumber, 0, 4, 1

   until iFileCount>=iNumberOfFiles do
	event_i "i", 1000, iFileCount, 1, iFirstTableNumber+iFileCount
	iFileCount = iFileCount+1
   enduntil

endin

instr 1000
   iTable = p4
   aOut loscil3 1, 1, iTable, 1, 0;
   outs aOut, aOut
endin 

</CsInstruments>
<CsScore>
i1 0 20
</CsScore>
</CsoundSynthesizer>



