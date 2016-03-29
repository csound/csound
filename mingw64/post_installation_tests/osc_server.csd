<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>

gi_osc OSCinit 7770

alwayson "osc_sender"
alwayson "osc_listener"

instr osc_sender
k_instrument init 1
k_time init 2
k_duration init 30
k_key init 4
k_velocity init 5
k_pan init 6
k_key jspline 25, 0.01, 2
k_key = int(k_key) + 50
k_trigger metro 0.5
OSCsend k_trigger, "localhost", 7770, "/osc_channel", "dddddd", k_instrument, k_time, k_duration, k_key, k_velocity, k_pan
printf "Sent:     k_instrument: %9.4f k_time: %9.4f k_duration: %9.4f k_key: %9.4f k_velocity: %9.4f k_pan: %9.4f\n\n", k_trigger, k_instrument, k_time, k_duration, k_key, k_velocity, k_pan
endin

instr osc_listener
k_instrument init 0
k_time init 0
k_duration init 0
k_key init 0
k_velocity init 0
k_pan init 0
k_answer OSClisten gi_osc, "/osc_channel", "dddddd", k_instrument, k_time, k_duration, k_key, k_velocity, k_pan
printk2 k_answer
printf "Received: k_instrument: %9.4f k_time: %9.4f k_duration: %9.4f k_key: %9.4f k_velocity: %9.4f k_pan: %9.4f\n", k_answer, k_instrument, k_time, k_duration, k_key, k_velocity, k_pan
endin

</CsInstruments>
<CsScore>
f 0 36000
</CsScore>
</CsoundSynthesizer>
