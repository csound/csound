#!/usr/bin/python
#####################################
#  oscilloscope example
#  
#   VL, 01/07

import csnd
from Tkinter import *
import display
import array

# window size, refresh interval and norm factor
window_size = 300
time_interval = 0.2
norm = 32768.0

# display callback
def callb(data):
    sig = array.array('f')
    cs  = data[0]
    disp = data[1]
    chn = data[2]
    cs.ChanOAGet(chn.cast(), 1)
    for i in range(0,cs.GetKsmps()):
      sig.append(chn[i]/norm)
    disp.draw(sig,time_interval*cs.GetSr())

# create & compile instance
cs = csnd.Csound()
cs.Compile("am.csd")

# create the thread object
perf = csnd.CsoundPerformanceThread(cs)

# display object
master = Tk()
disp = display.Oscilloscope(master, window_size, perf.Stop, "green", "black")

# samples array
chn = csnd.floatArray(cs.GetKsmps())
dat = (cs,disp,chn)

# set the callback
perf.SetProcessCallback(callb, dat)

# play
perf.Play()

# run the display
disp.mainloop()





