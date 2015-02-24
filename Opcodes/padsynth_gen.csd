<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=2
0dbfs=1000

	gispec_len init 2^18


	instr 1	;PadSynth
    ;                       p1 p2 p3          p4           p5      p6        p7    p8 p9  p10  p11
	gi_padsynth_1 ftgenonce 0, 0, gispec_len, "padsynth", 440,    60.96943, 0.09, 1, 2,  0.5, 0.7600046992, 0.6199994683, 0.9399998784, 0.4400023818, 0.0600003302, 0.8499968648, 0.0899999291, 0.8199964762, 0.3199984133, 0.9400014281, 0.3000001907, 0.120003365, 0.1799997687, 0.5200006366, 0.9300042987
	;gi_padsynth_1 ftgenonce 0, 0, gispec_len, "padsynth", 261.63, 56.04,    0.1,  1, 1,  0.4, 1, 0, .5, 0, .5
    istereo_phase = 1
    iphase random 0, 1
    iattack = 0.08
    idecay = 0.1
    isustain = 0.25
    irelease = 0.2
    kenv madsr iattack, idecay, isustain, irelease
    ifreq cpsmidinn p4
    iamp ampdb p5
    ibasefreq = 440 ; can be lower or higher frequency; close to played frequency is said to be best
    ibw_cents = 56.96943 ; width of the peaks, 100 is semitone

    if istereo_phase==0 then
	asig oscili iamp, ifreq*(sr/gispec_len/ibasefreq), gi_padsynth_1, iphase
	asig = asig*kenv
	aleft, aright pan2 asig, 0.5
	outs aleft, aright
    else
	asig1 oscili iamp, ifreq*(sr/gispec_len/ibasefreq), gi_padsynth_1, iphase
	asig2 oscili iamp, ifreq*(sr/gispec_len/ibasefreq), gi_padsynth_1, iphase+0.5
	outs asig1*kenv, asig2*kenv
    endif

	endin



</CsInstruments>

<CsScore>



i1 0 2 60.00 60
i1 + 2 72.00 60
i1 + 2 84.00 60

e
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
