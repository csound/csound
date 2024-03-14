import glob
import os, re
files = glob.glob('*.orc')
files.sort()
files.remove('ambisonic.orc')
files.remove('fluid.orc')
files.remove('fluidAllOut.orc')

for file in files:
  #print file
  handleorc = open(file, 'r')
  handlesco = open(file[:file.index('.orc')]+'.sco')
  #print file[:file.index('.sco')]+'.csd'
  newtext = '''<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent  MIDI in
-odac           -idac     -d       -M0  ;;;realtime output
; For Non-realtime ouput leave only the line below:
; -o ''' + file[:file.index('.orc')] + '''.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
'''
  commentorc = re.compile('/.*[0-9a-z]*\.orc.*/[\N\R]*')
  commentsco = re.compile('/.*[0-9a-z]*\.sco.*/[\N\R]*')
  orc = handleorc.read()
  orc = commentorc.sub('',orc)
  sco = handlesco.read()
  sco = commentsco.sub('',sco)

  newtext +=  orc + '\n</CsInstruments>\n<CsScore>\n' + sco + '\n</CsScore>\n</CsoundSynthesizer>\n'
  #print newtext
  dest = open(file[:file.index('.orc')]+'.csd','w')
  dest.write(newtext)
  dest.close()
  handleorc.close()
  handlesco.close()

