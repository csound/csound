<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xscanmap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
;the matrices can be found in /manual/examples

instr 1	; Plain scanned syntnesis
	; note - scanu display is turned off
a0    = 0							
      xscanu   1, .01, 6, 2, "128-stringcircularX", 4, 5, 2, .1, .1, -.01, .1, .5, 0, 0, a0, 0, 0  		
  a1  xscans   p4, cpspch(p5), 7, 0, 3
k1,k2 xscanmap 0, 1000, 1000, 64
      display  k1, .25 ; note - display is updated every second
      outs     a1, a1
endin

instr 2	; Scan synthesis with audio injection and dual scan paths
	; note - scanu display is turned off
ain   diskin2 "fox.wav",1,0,1	
ain   in
a0    = ain/10000 						
      xscanu   1, .01, 6, 2, "128,8-gridX", 14, 5, 2, .01, .05, -.05, .1, .5, 0, 0, a0, 0, 0 		
a1    xscans   p4, cpspch(p5), 7, 0, 2
a2    xscans   p4, cpspch(p6), 77, 0, 3
k1,k2 xscanmap 0, 1000, 1000, 127
      display  k2, .5  						; note - display is updated ever 500ms
      outs     a1,a2
endin 

</CsInstruments>
<CsScore>
; Initial condition
;f1 0 16 7 0 8 1 8 0
f1 0 128 7 0 64 1 64 0

; Masses
f2 0 128 -7 1 128 1

; Centering force
f4  0 128 -7 0 128 2
f14 0 128 -7 2 64 0 64 2

; Damping
f5 0 128 -7 1 128 1

; Initial velocity
f6 0 128 -7 -.0 128 .0

; Trajectories
f7 0 128 -5 .001 128 128
f77 0 128 -23 "128-spiral-8,16,128,2,1over2"

; Sine
f9 0 1024 10 1
;--------------------------------
; Note list
i1 0 10 .9 7.00                                  
s
i2 0 10 1  8.00 6.00
i2 0 10 1  7.00 8.05
e
</CsScore>
</CsoundSynthesizer>

