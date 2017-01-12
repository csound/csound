<CsoundSynthesizer>
<CsLicense>
TEST FOR ABLETON LINK OPCODES
Michael Gogins
12 January 2017
</CsLicense>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr=44100
kr=441
ksmps=100
nchnls=2
alwayson "LinkMonitor"

instr LinkMonitor
i_link link_create 72
printf_i "i_link: %g\n", 1, i_link
link_enable i_link, 1
k_trigger, k_beat, k_phase, k_time link_metro i_link, 4
k_peers link_peers i_link
k_tempo link_tempo_get i_link
printf "LinkMonitor: i_link: %g k_trigger: %f beat: %f k_phase: %f time: %f tempo: %f peers: %f\n", k_trigger, i_link, k_trigger, k_beat, k_phase, k_time, k_tempo, k_peers
endin

</CsInstruments>

<CsScore>
f 0 360
</CsScore>

</CsoundSynthesizer>
