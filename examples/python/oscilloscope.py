#!/usr/bin/python
#####################################
#  oscilloscope example
#  
#   VL, 01/07

import csnd6
from Tkinter import *
import display
import array
import threading

# window size, refresh interval and norm factor
window_size = 300
time_interval = .1
norm = 32768.0

lock = threading.Lock()
class drawThread(threading.Thread):
    def run(self):
      lock.acquire()
      self.disp.draw(self.sig,len(self.sig))
      lock.release()
    def __init__(self, disp, sig):
        threading.Thread.__init__(self)
        self.disp = disp
        self.sig = sig
        
# display callback
class Disp:
 def callb(self, dummy):
    cs  = self.data[0]
    disp = self.data[1]
    size = time_interval*cs.GetSr()
    for i in range(0,cs.GetKsmps()):
      self.sig.append(cs.GetSpoutSample(i,0)/norm)
    self.cnt += cs.GetKsmps()
    if(self.cnt >= size):
       t = drawThread(disp, self.sig)
       t.start()
       self.cnt = 0
       self.sig = array.array('d')
   
 def __init__(self,data):
      self.sig = array.array('d')
      self.data = data
      self.cnt = 0;

# create & compile instance
cs = csnd6.Csound()
cs.Compile("am.csd")

# create the thread object
perf = csnd6.CsoundPerformanceThread(cs)

# display object
master = Tk()
disp = display.Oscilloscope(master, window_size, perf.Stop, "green", "black")

dat = (cs,disp,master)
tes = Disp(dat)

# set the callback
perf.SetProcessCallback(tes.callb, None)

# play
perf.Play()

# run the display
disp.mainloop()





