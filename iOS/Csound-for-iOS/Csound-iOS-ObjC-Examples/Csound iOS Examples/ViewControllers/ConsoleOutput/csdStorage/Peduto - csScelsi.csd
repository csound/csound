;Csound_Scelsi
;Alfonso Peduto 2012

<CsoundSynthesizer>

<CsOptions>
-d -g
</CsOptions>

<CsInstruments>
sr 		= 		44100
ksmps 		= 		32
nchnls 		= 		2
0dbfs = 1

iwave ftgen 1, 0, 4096, 10, 1, .5, .33, .25,  .0, .1,  .1, .1

gasendL		init		0
gasendR		init		0

turnon  "reverber"
turnon  "generate1"
turnon  "generate2"
turnon  "generate3"
turnon  "generate4"


seed		0

;3-notes, medium-low velocity, low to medium frequencies
instr generate1

ininstr = 3
indx = 0

loop:
ivel  random 30, 50
ifreq random 48, 80
idur  random 30, 40

event_i "i", "strings", 0+rnd(6), idur, ifreq, ivel 
event_i "i", "strings", 60+rnd(10), idur, ifreq, ivel 
loop_lt indx, 1, ininstr, loop

endin


;6-note cluster background, low velocity, low frequencies
instr generate2

ininstr = 6
indx = 0

loop:
ivel  random 0, 30
ifreq random 30, 60
idur  random 60, 100

event_i "i", "strings", 20+rnd(12), idur, ifreq, ivel 
event_i "i", "strings", 90+rnd(12), idur, ifreq, ivel 
event_i "i", "strings", 225+rnd(12), idur, ifreq, ivel 
loop_lt indx, 1, ininstr, loop

endin


;sharp attacks, high velocity, short durations
instr generate3

ininstr = 15
indx = 0

loop:
ivel  random 90, 110
ifreq random 48, 80
idur  random 0.07, 0.6

event_i "i", "strings", 120+rnd(50), idur, ifreq, ivel 
loop_lt indx, 1, ininstr, loop

endin


;7-notes cluster background, high frequency, low velocity
instr generate4

ininstr = 7
indx = 0

loop:
ivel  random 0, 35
ifreq random 60, 90
idur  random 40, 80

event_i "i", "strings", 180+rnd(8), idur, ifreq, ivel 
loop_lt indx, 1, ininstr, loop

endin



instr strings
;Main
kdtn		jspline		0.05, 0.4, 0.8	
inote  = cpsmidinn(p4)
kcps transeg inote, p3*0.3, 2, inote -rnd(8) , p3*0.3 , 2 , inote +rnd(9), p3*0.4, 2, inote-rnd(2)
iamp  = (p5/127)/4
kctrl   linseg  0, p3*0.80, iamp, p3*0.20, 0  
amain   oscil   kctrl, kcps*semitone(kdtn), 1           
aflat   oscil   kctrl, kcps - .25, 1       
asharp   oscil   kctrl, kcps + .25, 1       
asig    =   amain + asharp + aflat

;LPF
asig butterlp asig, (p5)*10

;Panning
kpan		rspline		0,1,0.01,2
a1, a2	pan2		asig, kpan
		outs		a1, a2
			
gasendL		=		gasendL+a1
gasendR		=		gasendR+a2  
endin


		instr		reverber
aL, aR		reverbsc	gasendL,gasendR,0.85,10000
		outs		aL, aR
		clear		gasendL, gasendR
		endin

</CsInstruments>

<CsScore>
f 0 4000
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
