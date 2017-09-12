<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o timout.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

indx = 0
itim = p4				;change time for one step

clock: 
   timout 0, itim, time
   reinit clock

time:
   itmp table indx, 2, 0, 0, 1
   if itmp == 1 then
   print itmp
   event_i "i",2, 0, .1			;event has duration of .1 second
endif
indx = indx+1

endin

instr 2	;play it

kenv transeg 0.01, p3*0.25, 1, 1, p3*0.75, .5, 0.01
asig oscili kenv*.4, 400, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 1024 10 1	;sine
f 2 0 16 2 1 0 0 1 0 1 0 1 0 1 0 0 0 0 0 0 ;the rythm table

i1 0 10 .1
i1 + 10 .05
i1 + 10 .01
e
</CsScore>
</CsoundSynthesizer>
