<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o wrap.wav -W  ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr    1 ; Limit / Mirror / Wrap

igain    = p4				;gain
ilevl1   = p5				; + level
ilevl2   = p6				; - level
imode    = p7				;1 = limit, 2 = mirror, 3 = wrap

ain   soundin  "fox.wav"
ain   = ain*igain

if    imode = 1 goto limit
if    imode = 2 goto mirror

asig  wrap  ain, ilevl2, ilevl1
goto  outsignal

limit:
asig  limit  ain, ilevl2, ilevl1
goto  outsignal

mirror:
asig  mirror  ain, ilevl2, ilevl1
outsignal:

outs  asig*.5, asig*.5			;mind your speakers
  
endin

</CsInstruments>
<CsScore>

;           Gain  +Levl -Levl Mode
i1  0  3    4.00  .25  -1.00   1	;limit
i1  4  3    4.00  .25  -1.00   2	;mirror
i1  8  3    4.00  .25  -1.00   3	;wrap
e
</CsScore>
</CsoundSynthesizer>
