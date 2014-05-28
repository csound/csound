# -*- coding: utf-8 -*-

# Using code by Jacob Joaquin from:
# http://nbviewer.ipython.org/gist/jacobjoaquin/5535792
# http://nbviewer.ipython.org/gist/jacobjoaquin/5550729

# Support for python 2.6+ and 3
from __future__ import print_function
from __future__ import division

import csnd6

from pylab import *

class icsound:
    def __init__(self, sr = 44100, ksmps = 128, nchnls = 2, zerodbfs = 1.0, dacout = '', adcin = None):
        self._sr = sr
        self._ksmps = ksmps
        self._nchnls = nchnls
        self._0dbfs = zerodbfs
        self._dacout = dacout
        self._adcin = adcin
        self._cs = None
        self._csPerf = None
        self._prev_mess_newline = True
        self._printrt = False
        self._log = ''
        
    def _flush_messges(self):
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()

    def start_engine(self, sr = 44100, ksmps = 64, nchnls = zerodbfs=1.0):
        if self._cs and self._csPerf:
            print("icsound: Csound already running")
            return
        self._cs = csnd6.Csound()
        self._cs.CreateMessageBuffer(0)
        self._cs.SetOption('-odac' + self._dacout)
        self._sr = sr
        self._ksmps = ksmps
        self._nchnls = nchnls
        self._0dbfs = zdbfs
        self._cs.CompileOrc(
        '''sr = %i
        ksmps = %i
        nchnls = %i
        0dbfs = %f
        '''%(self._sr, self._ksmps, self._nchnls, self._0dbfs))
        self._cs.Start()
        self._csPerf = csnd6.CsoundPerformanceThread(self._cs)
        self._csPerf.Play()
        self._flush_messges()

    def stop_engine(self):
        # FIXME this is crashing occasionally, why?
        self._csPerf.Stop()
        
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()
#        self._cs.Stop()
        self._cs = None
        self._csPerf = None

    def send_score(self, score, printrt = False):
        self._printrt = printrt
        self._csPerf.InputMessage(score)
        self._flush_messges()

    def send_code(self, code):
        ret = self._cs.CompileOrc(code)
        error_text = ''
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            error_text += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()
        if ret:
            print(error_text)

    def make_table(self, tabnum, size, gen, *args):
        print(list(args))
        data = ', '.join(map(str, list(args)))
        data = 'gitemp ftgen %i, 0, %i, %i, '%(tabnum, size, gen) + data
        print(data)
        self.send_code(data)

    def get_table_data(self, t):
        length = self._cs.TableLength(t)
        tabarray = csnd6.doubleArray(length)
        self._cs.TableCopyOut(t, tabarray)
        table = []
    
        for i in xrange(length):
            table.append(tabarray[i])

        return table
        
    def fill_table(self, tabnum, arr):
        if type(arr) != np.ndarray and type(arr) != list and  type(arr) != tuple:
            raise TypeError("Argument is not array, list or tuple")
        if type(arr) == np.ndarray and arr.ndim > 1:
            raise ValueError("Only one dimensional arrays are valid")
        data = ', '.join(map(str, arr))
        data = 'gitemp ftgen %i, 0, %i, -2, '%(tabnum, len(arr)) + data
        print(data)
        self.send_code(data)
        
    def plot_table(self, tabnum):
        table_data = self.get_table_data(tabnum)
        table_length = len(table_data)
        fig, ax = subplots(figsize=(10, 6))
        ax.hlines(0, 0, table_length)
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
        ax.spines['bottom'].set_visible(False)
        ax.set_xticks(xrange(0, table_length + 1, int(table_length / 4)))
        ax.xaxis.set_ticks_position('bottom')
        ax.yaxis.set_ticks_position('left')
        ax.plot(table_data, color='black', lw=2)        
        xlim(0, table_length - 1)
        
    def set_channel(self, channel, value):
        self._cs.SetChannel(channel, value)
        
    def get_channel(self, channel):
        return self._cs.GetChannel(channel)
        
    def print_log(self):
        print(self._log)


# TODO:
# modify table data
# set python callback
# play numpy array directly
# have a way to save an audio file


if __name__ == "__main__":
    print("Please use import icsound")
    cs = icsound()
    cs.start_engine()

    #cs.make_table(5, 1024, 10, 1)
    #cs.fill_table(2, (1,2,3,4,5,2))
    #gen_table(1, 0, 8192, 10, *(map(lambda x: 1.0 / (x + 1), range(16 + 1))))
    cs.stop_engine()
