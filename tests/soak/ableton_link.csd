<CsoundSynthesizer>
<CsLicense>
Run the Ableton Link "LinkHut" example application, or some other 
Ableton Link peer, while you run this CSD to see what happens.
</CsLicense>
<CsOptions>
-m0 -d -odac 
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2

alwayson "LinkMonitor"

gi_peer link_create 72
gk_beat init 0

instr Beep
asignal oscils 20000, p4, 0
outs asignal, asignal
endin

instr TempoChange
link_tempo_set gi_peer, 80
endin

instr LinkEnable
i_enabled = p4
link_enable gi_peer, i_enabled
endin

instr LinkMonitor
i_kperiod_seconds = ksmps / sr
printf_i "kperiod: %9.6f seconds.\n", 1, i_kperiod_seconds
printf_i "gi_peer: %g\n", 1, gi_peer
link_enable gi_peer, 1
k_trigger, gk_beat, k_phase, k_time link_metro gi_peer, 1
k_peers link_peers gi_peer
k_tempo link_tempo_get gi_peer
k_enabled link_is_enabled gi_peer
k_hz = 1000
if floor(gk_beat % 4) == 0 then
k_hz = 3000
else
k_hz = 2000
endif
schedkwhen k_trigger, 0, 1, "Beep", 0, 0.01, k_hz
printf "LinkMonitor: gi_peer: %g k_enabled: %9.4f k_trigger: %9.4f beat: %9.4f k_phase: %9.4f time: %9.4f tempo: %9.4f peers: %3d\n", k_trigger, gi_peer, k_enabled, k_trigger, gk_beat, k_phase, k_time, k_tempo, k_peers
endin
</CsInstruments>
<CsScore>
f 0 360
i "TempoChange" 10 80
i "LinkEnable"  20 1 0
i "LinkEnable"  30 1 1
</CsScore>
</CsoundSynthesizer>
