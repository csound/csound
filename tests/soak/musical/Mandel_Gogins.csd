<CsoundSynthesizer>
<CsLicense>
MANDELBROT SET ORBIT SONIFICATION
Copyright (C) 2022 by Michael Gogins

This piece demonstrates how to generate Csound scores from orbits of points on 
or near the boundary of the Mandelbrot set.

See https://www.stefanbion.de/fraktal-generator/z-orbits.htm and similar 
resources in order to find such points by exploration.
</CsLicense>

<CsOptions>
-fodac -m34 -d -+msg_color=0
; -o Mandel_Gogins.wav -W ;;; for file output any platform 
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 128
nchnls = 2
0dbfs  = 1

alwayson "ReverbSC"
alwayson "MasterOutput"
connect "Plucked", "outleft", "ReverbSC", "inleft"
connect "Plucked", "outright", "ReverbSC", "inright"
connect "ReverbSC", "outleft", "MasterOutput", "inleft"
connect "ReverbSC", "outright", "MasterOutput", "inright"
gk_Plucked_midi_dynamic_range chnexport "gk_Plucked_midi_dynamic_range", 3
gk_Plucked_midi_dynamic_range init 30
gk_Plucked_space_left_to_right chnexport "gk_Plucked_space_left_to_right", 3
gk_Plucked_space_left_to_right init .5
gk_Plucked_level chnexport "gk_Plucked_level", 3
gk_Plucked_level init 0
gi_Plucked_sine ftgen 0, 0, 65537, 10, 1

instr Plucked
; Author: Michael Gogins
i_instrument = p1
i_time = p2
; Make indefinite notes last no longer than the physical decay.
i_physical_decay = 20
if p3 == -1 then
i_duration = i_physical_decay
else
i_duration = p3
endif
i_midi_key = p4
i_midi_dynamic_range = i(gk_Plucked_midi_dynamic_range)
i_midi_velocity = p5 ;* i_midi_dynamic_range / 127 + (63.5 - i_midi_dynamic_range / 2)
i_midi_velocity ampmidid i_midi_velocity, i_midi_dynamic_range
k_space_front_to_back = p6
if p7 == 0 then
k_space_left_to_right = gk_Plucked_space_left_to_right
else
k_space_left_to_right = p7
endif
k_space_bottom_to_top = p8
i_phase = p9
i_detune_cents = 1.5
i_detune = i_detune_cents / 100
i_frequency1 = cpsmidinn(i_midi_key - i_detune)
i_frequency2 = cpsmidinn(i_midi_key)
i_frequency3 = cpsmidinn(i_midi_key + i_detune)
; Adjust the following value until "overall amps" at the end of performance is about -6 dB.
i_overall_amps = 26 - 70
i_normalization = ampdb(-(i_overall_amps)) / 2
i_amplitude = ampdb(i_midi_velocity) * i_normalization
k_gain = ampdb(gk_Plucked_level)
asignal1 wgpluck2 0.1, 1.0, i_frequency1, 0.25, 0.222
asignal2 wgpluck2 0.1, 1.0, i_frequency2, 0.20, 0.223
asignal3 wgpluck2 0.1, 1.0, i_frequency3, 0.23, 0.225
a_signal = (asignal1 + asignal2 + asignal3)
; As with most instruments that are based upon an impulse delivered to a 
; resonator, there are two envelopes, one for the physical decay with a 
; fixed release ending at zero, and one with a release segment to remove 
; clicks from the attack and release.
;
; As with most software instruments that are modeled on an impulse exciting a 
; resonator, there should be two envelopes. The "physical" envelope must have a 
; fixed decay ending at zero.
i_declick_minimum = .001
i_attack = .001 / i_frequency2 + i_declick_minimum
i_exponent = 7
a_physical_envelope transeg 0,   i_attack, i_exponent,  1,   i_physical_decay, -i_exponent,  0
; The de-clicking envelope must have attack and release segments that damp 
; artifacts in the signal. The duration of these segments depends on 
; the behavior of the instrument, and may vary as a function of frequency.
i_declick_attack = i_attack
i_declick_release = i_declick_minimum * 2
; The end of the note must be extended _past_ the end of the release segment.
xtratim 1
a_declicking_envelope cossegr 0, i_declick_attack, 1,  i_duration, 1,  i_declick_release, 0
; The envelope of the instrument is the product of the physical envelope times 
; the declicking envelope. 
a_envelope = a_physical_envelope * a_declicking_envelope
; That envelope is then low-pass filtered to remove most discontinuities.
a_filtered_envelope tonex a_envelope, 40, 4
a_signal = a_signal * i_amplitude * a_filtered_envelope * k_gain *.001
#ifdef USE_SPATIALIZATION
a_spatial_reverb_send init 0
a_bsignal[] init 16
a_bsignal, a_spatial_reverb_send Spatialize a_signal, k_space_front_to_back, k_space_left_to_right, k_space_bottom_to_top
outletv "outbformat", a_bsignal
outleta "out", a_spatial_reverb_send
#else
a_out_left, a_out_right pan2 a_signal, k_space_left_to_right
outleta "outleft", a_out_left
outleta "outright", a_out_right
#endif
prints "%-24s i %9.4f t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f #%3d\n", nstrstr(p1), p1, p2, p3, p4, p5, p7, active(p1)
endin

gk_ReverbSC_feedback chnexport "gk_ReverbSC_feedback", 3
gk_ReverbSC_wet chnexport "gk_ReverbSC_wet", 3
gi_ReverbSC_delay_modulation chnexport "gi_ReverbSC_delay_modulation", 3
gk_ReverbSC_frequency_cutoff chnexport "gk_ReverbSC_frequency_cutoff", 3
gk_ReverbSC_feedback init 0.78
gk_ReverbSC_wet init 0.5
gi_ReverbSC_delay_modulation init 0.0075
gk_ReverbSC_frequency_cutoff init 15000
instr ReverbSC
gk_ReverbSC_dry = 1.0 - gk_ReverbSC_wet
aleftin init 0
arightin init 0
aleftout init 0
arightout init 0
aleftin inleta "inleft"
arightin inleta "inright"
aleftout, arightout reverbsc aleftin, arightin, gk_ReverbSC_feedback, gk_ReverbSC_frequency_cutoff, sr, gi_ReverbSC_delay_modulation
aleftoutmix = aleftin * gk_ReverbSC_dry + aleftout * gk_ReverbSC_wet
arightoutmix = arightin * gk_ReverbSC_dry + arightout * gk_ReverbSC_wet
outleta "outleft", aleftoutmix
outleta "outright", arightoutmix
prints "%-24s i %9.4f t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f #%3d\n", nstrstr(p1), p1, p2, p3, p4, p5, p7, active(p1)
endin

gk_MasterOutput_level chnexport "gk_MasterOutput_level", 3 ; 0
gS_MasterOutput_filename chnexport "gS_MasterOutput_filename", 3 ; ""
gk_MasterOutput_level init 0
gS_MasterOutput_filename init ""
instr MasterOutput
aleft inleta "inleft"
aright inleta "inright"
k_gain = ampdb(gk_MasterOutput_level)
printks2 "Master gain: %f\n", k_gain
iamp init 1
aleft butterlp aleft, 18000
aright butterlp aright, 18000
outs aleft * k_gain, aright * k_gain
; We want something that will play on my phone.
i_amplitude_adjustment = ampdbfs(-3) / 32767
i_filename_length strlen gS_MasterOutput_filename
if i_filename_length > 0 then
prints sprintf("Output filename: %s\n", gS_MasterOutput_filename)
fout gS_MasterOutput_filename, 18, aleft * i_amplitude_adjustment, aright * i_amplitude_adjustment
endif
prints "%-24s i %9.4f t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f #%3d\n", nstrstr(p1), p1, p2, p3, p4, p5, p7, active(p1)
endin

; Generates notes based on an orbit of a point in the Mandelbrot set.
; Points on the _boundary_ of the set orbit on a strange attractor.
; Points _near_ the boundary are attracted to a fixed point, or to 
; infinity.
; Implements z <= z^2 + c.
opcode mandelbrot_orbit, kk, kkkk
; a     b      
; c     d
kk_z_r, kk_z_i, kk_c_r, kk_c_i xin
; z1z2 = (ac - bd) + i(ad + bc)
kk_z1_r = kk_z_r * kk_z_r - kk_z_i * kk_z_i
kk_z1_i = kk_z_r * kk_z_i + kk_z_i * kk_z_r
kk_z1_r = kk_z1_r + kk_c_r
kk_z1_i = kk_z1_i + kk_c_i
xout kk_z1_r, kk_z1_i 
endop

; p4 is the real (or x) coordinate of the point, and
; p5 is the imaginary (or y) coordinate of the point.  
instr MandelbrotOrbit
kk_c_r init p4
kk_c_i init p5
kk_z_r init 0
kk_z_i init 0
kk_z1_r init 0
kk_z1_c init 0
i_interval init 4
k_iteration init 0
k_trigger metro i_interval 
if k_trigger == k(1) then
k_iteration += 1
kk_z1_r, kk_z1_i mandelbrot_orbit kk_z_r, kk_z_i, kk_c_r, kk_c_i
kk_z_r = kk_z1_r
kk_z_i = kk_z1_i
println "iteration: %4d kk_z_r: %9.4f kk_z_i: %9.4f", k_iteration, kk_z_r, kk_z_i
k_instrument = 1
k_time = 0
k_duration = 1.5 / i_interval
k_key_1 = floor(60 + kk_z_r * 30)
k_key_2 = floor(60 + kk_z_i * 30)
schedulek k_instrument, k_time, k_duration, k_key_1, 70, 0, .25
schedulek k_instrument, k_time, k_duration, k_key_2, 70, 0, .75
endif
endin

</CsInstruments>
<CsScore>
i "MandelbrotOrbit"  1 15 -.55 .44
i "MandelbrotOrbit" 20 15  .3  .4
i "MandelbrotOrbit" 40 15  .3  .1
</CsScore>
</CsoundSynthesizer>