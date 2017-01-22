<CsoundSynthesizer>
<CsOptions>
-o dac
-d
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

giWet		ftgen	0,0,1024,-7,0,512,1,512,1
giDry		ftgen	0,0,1024,-7,1,512,1,512,0

turnon 1

    opcode PitchShifter, aa, aakkk
    
aL, aR, kpitch, kfine, kfeedback xin 

		setksmps 64

ifftsize    = 1024
ihopsize    = 256
kscal       = octave((int(kpitch)/12)+kfine)

aOutL       init 0		
aOutR       init 0

fsig1L      pvsanal aL+(aOutL*kfeedback), ifftsize, ihopsize, ifftsize, 0
fsig1R      pvsanal aR+(aOutR*kfeedback), ifftsize, ihopsize, ifftsize*2, 0
fsig2L      pvscale fsig1L, kscal
fsig2R      pvscale fsig1R, kscal
aOutL       pvsynth fsig2L
aOutR       pvsynth fsig2R

            xout aOutL, aOutR
    endop

    instr 1 ;PITCH SHIFTER

a1, a2	ins

kmix        chnget "mix"
kpitch      chnget "pitch"
kpitch      scale kpitch, 12, -12
kfine       = 0      ;chnget "pitchshifter_fine"
kfine       scale kfine, 0.083333, -0.083333
kfeedback   = 0      ;chnget "pitchshifter_feedback"

kWet		table	kmix, giWet, 100
kDry       	table	kmix, giDry, 100	

aOutL, aOutR PitchShifter a1, a2, kpitch, kfine, kfeedback

aOutL		= (aOutL * kmix) + (a1 * (1.0 - kmix))
aOutR		= (aOutR * kmix) + (a2 * (1.0 - kmix))

		outs aOutL, aOutR
    
    endin


</CsInstruments>
<CsScore>
f0 36000

</CsScore>
</CsoundSynthesizer>
