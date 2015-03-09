<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=2
0dbfs=2000

	gispec_len init 2^18

	instr 1
    prints "Plain sine for frequency/amplitude/distortion comparison.\n"
	gi_padsynth_1 ftgenonce 0, 0, gispec_len, 10, 1
    iattack = 0.08
    idecay = 0.1
    isustain = 0.25
    irelease = 0.2
    aenv madsr iattack, idecay, isustain, irelease
    ifreq cpsmidinn p4
    iamp ampdb p5
    ibasefreq = 440 ; can be lower or higher frequency; close to played frequency is said to be best
    ibw_cents = 56.96943 ; width of the peaks, 100 is semitone
	asig poscil iamp, ifreq, gi_padsynth_1
    asig = aenv * asig
	aleft, aright pan2 asig, 0.5
	outs aleft, aright
	endin

	instr 2
    prints "PadSynth with sine tone.\n"
    ibasehz = 261.625565
    ;                       p1 p2 p3          p4           p5       p6  p7    p8 p9  p10  p11
	gi_padsynth_1 ftgenonce 0, 0, gispec_len, "padsynth", ibasehz, p6, 0.0,  1, 1,  1.0, 1
    iattack = 0.08
    idecay = 0.1
    isustain = 0.25
    irelease = 0.2
    aenv madsr iattack, idecay, isustain, irelease
    ifreq cpsmidinn p4
    iamp ampdb p5
	asig poscil iamp, ifreq*(sr/gispec_len/ibasehz), gi_padsynth_1
    asig = aenv * asig
	aleft, aright pan2 asig, 0.5
	outs aleft, aright
	endin

	instr 3
    prints "PadSynth with harmonics.\n"
    ibasehz = 261.625565
    ;                       p1 p2 p3          p4           p5       p6  p7 p8 p9  p10  p11
	gi_padsynth_1 ftgenonce 0, 0, gispec_len, "padsynth", ibasehz, p6, 1, 1, 1,  1, 0.7600046992, 0.6199994683, 0.9399998784, 0.4400023818, 0.0600003302, 0.8499968648, 0.0899999291, 0.8199964762, 0.3199984133, 0.9400014281, 0.3000001907, 0.120003365, 0.1799997687, 0.5200006366, 0.9300042987
    iattack = 0.08
    idecay = 0.1
    isustain = 0.25
    irelease = 0.2
    aenv madsr iattack, idecay, isustain, irelease
    ifreq cpsmidinn p4
    iamp ampdb p5
	asig poscil iamp, ifreq*(sr/gispec_len/ibasehz), gi_padsynth_1
    asig = aenv * asig
	aleft, aright pan2 asig, 0.5
	outs aleft, aright
	endin

	instr 4
    prints "PadSynth with inharmonic partials.\n"
    ibasehz = 261.625565
    ;                       p1 p2 p3          p4           p5       p6  p7 p8 p9  p10  p11
	gi_padsynth_1 ftgenonce 0, 0, gispec_len, "padsynth", ibasehz, p6, 1, 2, 3,  1, 0.7600046992, 0.6199994683, 0.9399998784, 0.4400023818, 0.0600003302, 0.8499968648, 0.0899999291, 0.8199964762, 0.3199984133, 0.9400014281, 0.3000001907, 0.120003365, 0.1799997687, 0.5200006366, 0.9300042987
    iattack = 0.08
    idecay = 0.1
    isustain = 0.25
    irelease = 0.2
    aenv madsr iattack, idecay, isustain, irelease
    ifreq cpsmidinn p4
    iamp ampdb p5
	asig poscil iamp, ifreq*(sr/gispec_len/ibasehz), gi_padsynth_1
    asig = aenv * asig
	aleft, aright pan2 asig, 0.5
	outs aleft, aright
	endin


</CsInstruments>

<CsScore>



i1  0 2 60.00 60
i1  + 2 72.00 60
i1  + 2 84.00 60

i2  7 2 60.00 60 0.3
i2  + 2 72.00 60 0.3
i2  + 2 84.00 60 0.3
i2  + 2 60.00 60 25
i2  + 2 72.00 60 25
i2  + 2 84.00 60 25
i2  + 2 60.00 60 55
i2  + 2 72.00 60 55
i2  + 2 84.00 60 55

i3 26 2 60.00 60 0.3
i3  + 2 72.00 60 0.3
i3  + 2 84.00 60 0.3
i3  + 2 60.00 60 25
i3  + 2 72.00 60 25
i3  + 2 84.00 60 25
i3  + 2 60.00 60 55
i3  + 2 72.00 60 55
i3  + 2 84.00 60 55

i4 45 2 60.00 60 0.3
i4  + 2 72.00 60 0.3
i4  + 2 84.00 60 0.3
i4  + 2 60.00 60 25
i4  + 2 72.00 60 25
i4  + 2 84.00 60 25
i4  + 2 60.00 60 55
i4  + 2 72.00 60 55
i4  + 2 84.00 60 55

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
