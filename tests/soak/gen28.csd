<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen28.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 4
  
ga1	init	0
ga2	init	0
ga3	init	0
ga4	init	0

instr 1	;uses GEN28 file "move", as found in /manual/examples

kx    init 0
ky    init 0
ktime line  0, 5, 5				;same time as in table 1 (="move")
asig  diskin2 "beats.wav", 1, 0, 1		;sound source is looped
a1, a2, a3, a4 space asig, 1, ktime, .1, kx, ky	;use table 1 = GEN28
ar1, ar2, ar3, ar4 spsend			;send to reverb

ga1  = ga1+ar1
ga2  = ga2+ar2
ga3  = ga3+ar3
ga4  = ga4+ar4
     outq a1, a2, a3, a4

endin

instr 99 ; reverb instrument

a1 reverb2 ga1, 2.5, .5
a2 reverb2 ga2, 2.5, .5
a3 reverb2 ga3, 2.5, .5
a4 reverb2 ga4, 2.5, .5
   outq	a1, a2, a3, a4

ga1=0	
ga2=0
ga3=0
ga4=0

endin
</CsInstruments>
<CsScore>
f1 0 0 28 "move"

i1 0 5		;same time as ktime
i 99 0 10	;keep reverb active
e
</CsScore>
</CsoundSynthesizer>
