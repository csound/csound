<CsoundSynthesizer>
<CsOptions>
-o dac
-M0
-d
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

massign 0, 1

ga1 init 0


opcode	cpsmid, k, k

kmid	xin

#define MIDI2CPS(xmidi) # (440.0*exp(log(2.0)*(($xmidi)-69.0)/12.0)) #
kcps	=	$MIDI2CPS(kmid)

	xout	kcps

		endop


instr 1

midinoteonkey p4, p5

kpch cpsmid p4

iattack chnget "attack" 
idecay chnget "decay" 
isustain chnget "sustain" 
irelease chnget "release"

k2 linsegr 0, iattack, 1, idecay, isustain, irelease, 0
a1 vco2 k2 * .2, kpch

ga1 = ga1 + a1

endin

instr 2

kcutoff chnget "cutoff"
kresonance chnget "resonance"

a1 moogladder ga1, kcutoff, kresonance

aL, aR reverbsc a1, a1, .72, 5000

outs aL, aR

ga1 = 0

endin

instr allNotesOff
turnoff2 1, 0, 1
turnoff
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i2 0 360000
 
</CsScore>
</CsoundSynthesizer>
