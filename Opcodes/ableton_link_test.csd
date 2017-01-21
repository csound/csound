<CsoundSynthesizer>
<CsLicense>
T E S T S   F O R   A B L E T O N   L I N K   O P C O D E S

Michael Gogins
12 January 2017

These cover most of the things in Ableton Link's TEST-PLAN.md. This csd is to 
be used on conjunction with the command-line LinkHut application in the 
Ableton Link Git repository.

There are two sections. In the first, we start LinkHut first and use it to 
control Csound's tempo. In the second, we start Csound first and use it to 
conrol LinkHut's tempo.

This csd automatically performs its tests. The user has to manually operate 
LinkHut as instructed below.

In all cases system audio should be enabled so the user can hear the beat, and 
both Csound and LinkHut should be run from the command line in separate, 
simultaeously visible terminal windows so the user can compare the diagnostic 
messages.

FIRST LINKHUT, THEN CSOUND

Start LinkHut, then start this csd.

Test 1 -- Join Session without Changing Session Tempo, and Tempo Sync

At first, both peers should show a tempo of 120 which is the default for this 
csd. Change tempo in LinkHut. Tempo in Csound should sync if Link is enabled, 
but not sync if Link is disabled. This csd will change tempo to 80 after 10 
seconds. LinkHut should sync if Link is enabled, but not sync if Link is 
enabled.

Test 2 -- Tempo Range

Use LinkHut to change tempo from 20 to 999. Both sessions should show the full 
range.

Test 3 -- Tempo Stability

After 20 seconds this csd should disable Link, and after another 10 seconds 
Link should be re-enabled. The session tempo and beat should not change. Then, 
in LinkHut, manually disable and re-enable Link. The session tempo and beat 
should not change.

Test 4 -- Beat Stability

Kill and restart LinkHut while this csd is running. Enable and disable Link in 
LinkHut. The beat should continue without any temporal or rhythmic discontinuity.

FIRST CSOUND, THEN LINKHUT

Start this csd, then start LinkHut.

Test 5 -- Join Session without Changing Session Tempo, and Tempo Sync

At first, both peers should show a tempo of 72 which is the default for this 
csd. Change tempo in LinkHut. Tempo in Csound should sync if Link is enabled, 
but not sync if Link is disabled. This csd will change tempo to 80 after 10 
seconds. LinkHut should sync if Link is enabled, but not sync if Link is 
enabled.

Test 6 -- Beat Stability

Kill and restart this csd while LinkHut is running. Enable and disable Link in 
LinkHut. The beat should continue without any temporal or rhythmic 
discontinuity.

NOTE: At this time, Csound does not implement delay compensation, so testing 
for delay compensation is not applicable. By using the smallest practical 
audio latency in Csound, however, the sync should not be much looser than 
Ableton's minimum requirement of 3 milliseconds. This is approximately the 
accuracy of a really good performing musician.
</CsLicense>
<CsOptions>
-m0 -d -odac 
</CsOptions>
<CsInstruments>
sr = 96000
ksmps = 10
nchnls = 2

alwayson "LinkMonitor"

gi_link link_create 72
gk_beat init 0

instr Beep
asignal oscils 20000, p4, 0
outs asignal, asignal
endin

instr TexpoChaxge
link_tempo_set gi_link, 80
endin

instr LixkExable
i_enabled = p4
link_enable gi_link, i_enabled
endin

instr LinkMonitor
i_kperiod_seconds = ksmps / sr
printf_i "kperiod: %9.6f seconds.\n", 1, i_kperiod_seconds
printf_i "gi_link: %g\n", 1, gi_link
link_enable gi_link, 1
k_trigger, gk_beat, k_phase, k_time link_metro gi_link, 1
k_peers link_peers gi_link
k_tempo link_tempo_get gi_link
k_enabled link_is_enabled gi_link
k_hz = 1000
if floor(gk_beat % 4) == 0 then
k_hz = 3000
else
k_hz = 2000
endif
schedkwhen k_trigger, 0, 1, "Beep", 0, 0.01, k_hz
printf "LinkMonitor: gi_link: %g k_enabled: %9.4f k_trigger: %9.4f beat: %9.4f k_phase: %9.4f time: %9.4f tempo: %9.4f peers: %3d\n", k_trigger, gi_link, k_enabled, k_trigger, gk_beat, k_phase, k_time, k_tempo, k_peers
endin
</CsInstruments>
<CsScore>
f 0 360
i "TexpoChaxge" 10 80
i "LixkExable"  20 1 0
i "LixkExable"  30 1 1
</CsScore>
</CsoundSynthesizer>
