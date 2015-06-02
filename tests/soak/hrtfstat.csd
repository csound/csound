<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o hrtfstat.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
    
gasrc init 0

instr 1	;a plucked string

kamp = p4
kcps = cpspch(p5)
icps = cpspch(p5)
a1 pluck kamp, kcps, icps, 0, 1

gasrc = a1

endin

instr 10;uses output from instr1 as source

aleft,aright hrtfstat gasrc, 90,0, "hrtf-44100-left.dat","hrtf-44100-right.dat"
             outs     aleft, aright

clear gasrc
endin

</CsInstruments>
<CsScore>

i1 0 2 .7 8.00	; Play Instrument 1: a plucked string
i1 .5 2 .7 8.00
i1 1 2 .7 8.00
i1 2 2 .7 7.00

i10 0 12	; Play Instrument 10 for 2 seconds.

</CsScore>
</CsoundSynthesizer>
