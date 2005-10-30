import sys
import csnd
help(csnd)
help(csnd.Csound)
help(csnd.CsoundFile)
help(csnd.CppSound)
csound = csnd.Csound()
csound.Compile('-o', 'dac', '../examples/trapped.csd')
while not csound.PerformBuffer():
    pass
csound.Reset()

