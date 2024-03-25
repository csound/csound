<CsoundSynthesizer>
<CsOptions>
-odac ; for RT audio
</CsOptions>
        <CsInstruments>

sr = 44100
ksmps = 16
nchnls = 2
0dbfs = 1.0

ga1 init 0   ; delay aux

instr 1 ; main triggering instrument
kndx init 0
kndx2 init -1
; the clicks are 1/16th notes @ 137 BPM
kT metro2 (4*137/60), p4, -1
if (kT == 0) goto Halt
	k1 table kndx, 1, 0, 0, 1
	kndx += 1
	if (k1 == 0) goto Next
		event "i", 2, 0, 0.35, k1
Next: 
; positive amplitude values of down-beat clicks 
; are used to trigger kick (instr 3)
if (kT < 1) goto Halt
	kndx2 += 1
	kndx2 wrap kndx2, 0, 2 
 	if kndx2 != 0 goto Halt
	event "i", 3, 0, 0.2
Halt:
endin

instr 2  ; simple subtractive bass
kAE linsegr 0, 0.005, 1, p3/2, .7, .04, 0, .04, 0
kFE linsegr 1, 0.005, 2, p3/2, .7, .04, .1, .04, .1
ifr = cpspch(p4)
a1 vco2 1, ifr
a2 vco2 1, ifr * 1.005
a3 vco2 1, ifr * 0.993
aM = (a1+a2+a3)/3
aM moogvcf aM*kAE, 1000 * kFE, 0.5
outs aM, aM
ga1 += aM * 0.25
endin

instr 3 ; simple techno kick
k1 linseg 200, p3, 10
k2 linseg 0,0.001,1,0.25,0
a1 oscil 0.3*k2, k1
outs a1,a1
endin

instr 99 ; feedback delay for bass
a1 delayr 0.5
ab deltap 0.33
   delayw ga1 + ab*0.3
outs ab,ab
ga1 = 0
endin

</CsInstruments>
<CsScore>
t 0 137 
; ftable1 is the bass sequence to played with various swings
f1 0 16 -2 6.00 0 0 7.00 0 0 6.00 0 6.00 0 0 7.00 0 7.01 6.00 0
; 4 measure pattern of different swinging 
i1 0 16 0.5
i1 + .  0.65
i1 + .  0.4
i99 0 46
</CsScore>
</CsoundSynthesizer>
