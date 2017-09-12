#####################
# Python callbacks example
# needs cb.csd
# VL, 2007

import csnd6
import time

# create & compile instance
cs = csnd6.Csound()
cs.Compile("cb.csd")

# define callback
def callback(csound):
    a = csound.GetChannel("freq")
    if(a < 1000): a = a+0.1
    else: a = 100.0
    csound.SetChannel("freq", a)

# set an initial value for freq
cs.SetChannel("freq", 100)

# create the thread object
perf = csnd6.CsoundPerformanceThread(cs)

# set the callback
perf.SetProcessCallback(callback, cs)

# play
perf.Play()

# wait for 30 secs
time.sleep(30)

