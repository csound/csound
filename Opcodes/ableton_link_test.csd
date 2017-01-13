<CsoundSynthesizer>
<CsLicense>
T E S T S   F O R   A B L E T O N   L I N K   O P C O D E S
Michael Gogins
12 January 2017

These test most of the things in Ableton Link's TEST-PLAN.md.

Test 1 -- Tempo sync

Run LinkHut and enable it. Run this csd. This csd should show a tempo of 120. 
Change tempo in LinkHut. Tempo in Csound should sync. This csd will change 
tempo to 80 after 10 seconds. LinkHut should sync.

Test 2 -- Join session without changing session tempo.

Run LinkHut and enable it (defaults to 120 bpm). Run this csd (defaults to 72 
bpm). The tempo should be 120 bpm in both sessions.

Run this csd. Run LinkHut. The tempo should be 72 bpm in both sessions.

Test 3 -- Tempo range

Run LinkHut and enable it. Run this csd. Use LinkHut to change tempo from 20 
to 999. Both sessions should show the full range.

Test 4 -- Tempo stability

Run this csd. After 20 seconds Link should be disabled, and after another 10 
seconds Link should be re-enabled. The tempo should not change.

Run LinkHut. Enable and disable Link. The tempo should not change.

Test 5 -- Beat stability

Kill LinkHut. Run this csd. The beat should continue with any time or rhythmic discontinuity as Link is enabled and disabled by this csd.

Restart LinkHut while this csd is running. Enable and disable Link in LinkHut. The beat should continue without any temporal or rhythmic discontinuity.

NOTE: At this time, Csound does not implement delay compensation, so testing 
for delay compensation is not applicable. By using the smallest practical 
audio latency in Csound, however, the sync should not be much looser than 
Ableton's minimum requirement of 3 milliseconds. This is approximately the 
accuracy of a really good performing musician.

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

instr Beep
asignal oscils 10000, 880, 0
outs asignal, asignal
endin

instr LinkMonitor
i_link link_create 72
printf_i "i_link: %g\n", 1, i_link
link_enable i_link, 1
k_trigger, k_beat, k_phase, k_time link_metro i_link, 4
k_peers link_peers i_link
k_tempo link_tempo_get i_link
k_enabled link_is_enabled i_link
schedkwhen k_trigger, 0, 1, "Beep", 0, 0.01
printf "LinkMonitor: i_link: %d k_enabled: %f k_trigger: %f beat: %f k_phase: %f time: %f tempo: %f peers: %f\n", k_trigger, i_link, k_enabled, k_trigger, k_beat, k_phase, k_time, k_tempo, k_peers
endin

</CsInstruments>

<CsScore>
f 0 360
</CsScore>

</CsoundSynthesizer>
