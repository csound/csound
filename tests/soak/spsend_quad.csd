<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d   ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o space_quad.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>
sr = 44100
kr = 4410
ksmps = 10
nchnls = 4
  
ga1	init	0
ga2	init	0
ga3	init	0
ga4	init	0

instr 1

asig	diskin2	"beats.wav", 1
a1, a2, a3, a4	space	asig, 0, 0, .1, p4, p5	; take position values from p4, p5
ar1, ar2, ar3, ar4	spsend

ga1  = ga1+ar1
ga2  = ga2+ar2
ga3  = ga3+ar3
ga4  = ga4+ar4
	outq	a1, a2, a3, a4
endin

instr 99 ; reverb instrument

a1	reverb2	ga1, 2.5, .5
a2	reverb2	ga2, 2.5, .5
a3	reverb2  ga3, 2.5, .5
a4	reverb2  ga4, 2.5, .5
	outq	a1, a2, a3, a4

ga1=0
ga2=0
ga3=0
ga4=0

endin

</CsInstruments>
<CsScore>
;place the sound in the left speaker and near
  i1 0 1 -1 1
;place the sound in the right speaker and far
  i1 1 1 45 45
;place the sound equally between left and right and in the middle ground distance
  i1 2 1 0 12

  i 99 0 5
  e
</CsScore>
</CsoundSynthesizer>