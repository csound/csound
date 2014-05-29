# -*- coding: utf-8 -*-

# Copyright (C) 2010, 2014 Francois Pinot, Andres Cabrera, Jacob Joaquin
#
# This code is free software; you can redistribute it
# and/or modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this code; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
#

# Using code by Jacob Joaquin from:
# http://nbviewer.ipython.org/gist/jacobjoaquin/5535792
# http://nbviewer.ipython.org/gist/jacobjoaquin/5550729

# See Francois Pinot's Journal article:
# http://csounds.com/journal/issue14/realtimeCsoundPython.html

# Support for python 2.6+ and 3
from __future__ import print_function
from __future__ import division

import csnd6
from pylab import *

use_ctypes = False

try:
    import ctypes
    use_ctypes = True
except:
    print("ctypes not available. Some operations will be slow or unavailble.")
    use_ctypes = False

class icsound:
    def __init__(self):
        self._cs = None
        self._csPerf = None
        self._prev_mess_newline = True
        self._log = ''
        self._verbose = False
        self._myfltsize = csnd6.csoundGetSizeOfMYFLT()

    def start_engine(self, sr = 44100, ksmps = 64, nchnls = 2, zerodbfs=1.0, dacout = '', adcin = None):
        if self._cs and self._csPerf:
            print("icsound: Csound already running")
            return
        self._sr = sr
        self._ksmps = ksmps
        self._nchnls = nchnls
        self._0dbfs = zerodbfs
        self._dacout = dacout
        self._adcin = adcin
        self._cs = csnd6.Csound()
        self._cs.CreateMessageBuffer(0)
        self._cs.SetOption('-odac' + self._dacout)
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
        '''Stop Csound thread'''
        # FIXME this is crashing occasionally, why?
        self._csPerf.Stop()
        
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()
#        self._cs.Stop()
        self._cs = None
        self._csPerf = None

    def send_score(self, score):
        '''Send score text to Csound'''
        self._csPerf.InputMessage(score)
        self._flush_messges()

    def score_event(self, eventType, pvals, absp2mode = 0):
        """Send a score event to csound"""
        if not use_ctypes:
            print("score_event() not available without ctypes.")
        n = len(pvals)
        if self._myfltsize == 8:
            pfields = csnd6.doubleArray(n)
        else:
            pfields = csnd6.floatArray(n)
        for i in xrange(n):
            pfields[i] = pvals[i]
        self._cs.ScoreEvent(absp2mode, eventType, n, pfields)

    def send_code(self, code):
        '''Send csound orchestra code. Instruments can be sent to the Csound
        engine using this function'''
        ret = self._cs.CompileOrc(code)
        error_text = ''
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            error_text += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()
        if ret:
            print(error_text)

    def make_table(self, tabnum, size, gen, *args):
        '''Create a Csound table using Csound's GEN functions.'''

        data = 'gitemp_ ftgen %i, 0, %i, %i, '%(tabnum, size, gen)
        data += ', '.join(map(str, list(args)))
        self._debug_print(data)
        self.send_code(data)
        
    def fill_table(self, tabnum, arr):
        '''Create or fill a Csound f-table with the values passed in the arr
        variable. This variable can be a python list or a numpy array. Having ctypes
        installed will make this function significantly faster'''

        if type(arr) != np.ndarray and type(arr) != list and  type(arr) != tuple:
            raise TypeError("Argument is not array, list or tuple")
        if type(arr) == np.ndarray and arr.ndim > 1:
            raise ValueError("Only one dimensional arrays are valid")

        if use_ctypes:
            tbl = csnd6.CsoundMYFLTArray()
            # FIXME what about guard point?
            tblSize = self._cs.GetTable(tbl.GetPtr(), tabnum) # Needs + 1 due to guard point
            if tblSize <= 0:
                self._debug_print("Creating table ", tabnum)
                self.make_table(tabnum, len(arr), 2, 0)
                tblSize = self._cs.GetTable(tbl.GetPtr(), tabnum)
            elif not tblSize == len(arr):
                self._debug_print("Resizing table", tabnum, "from", tblSize, "to", len(arr))
                self.make_table(tabnum, len(arr), 2, 0)
                tblSize = self._cs.GetTable(tbl.GetPtr(), tabnum)

            if self._myfltsize == 8:
                myflt = ctypes.c_double
            else:
                myflt = ctypes.c_float
            cs_ftable = ctypes.ARRAY(myflt, tblSize).from_address(tbl.GetPtr(0).__long__())
            if type(arr) == np.ndarray:
                src_array = arr.astype(np.float64).ctypes.data_as(ctypes.POINTER(myflt))
            else:
                fltarray = myflt * len(arr)
                src_array = fltarray(*arr)
            ctypes.memmove(cs_ftable, src_array, len(arr) * self._myfltsize)
        else:
            data = ', '.join(map(str, arr))
            data = 'gitemp ftgen %i, 0, %i, -2, '%(tabnum, len(arr)) + data
            self.send_code(data)

    def get_table_data(self, t):
        '''Get the values of a Csound f-table as a list'''
        length = self._cs.TableLength(t)
        if length < 0:
            raise(ValueError("ftable number %d does not exist!" % (num)))
        tabarray = csnd6.doubleArray(length)
        self._cs.TableCopyOut(t, tabarray)
        table = []
        if use_ctypes:
            if self._myfltsize == 8:
                flttype = np.float32
            else:
                flttype = np.float64
            table = np.fromiter(tabarray, dtype=flttype, count=length)
        else:
            for i in xrange(length):
                table.append(tabarray[i])
        return table
        
    def plot_table(self, tabnum):
        '''Plot a Csound f-table using matplotlib'''
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
        '''Set a channel value'''
        self._cs.SetChannel(channel, value)
        
    def get_channel(self, channel):
        return self._cs.GetChannel(channel)
        
    def print_log(self):
        '''Print the complete log of the current instance of Csound.'''
        print(self._log)

    def set_verbose(self,verbose=True):
        self._verbose = verbose

    def _flush_messges(self):
        for i in range(self._cs.GetMessageCnt()):
            self._log += self._cs.GetFirstMessage()
            self._cs.PopFirstMessage()

    def _debug_print(self, *text):
        if self._verbose:
            print(text)

# TODO:
# modify table data
# set python callback
# play numpy array directly
# have a way to save an audio file
# implement setting adc and dac devices for engine
# get interfaces list
# get_table_data set limits (min, max)


if __name__ == "__main__":
    print("Please use import icsound")
    cs = icsound()
    cs.start_engine()

    #cs.make_table(5, 1024, 10, 1)
    #cs.fill_table(2, (1,2,3,4,5,2))
    #gen_table(1, 0, 8192, 10, *(map(lambda x: 1.0 / (x + 1), range(16 + 1))))
    cs.stop_engine()
