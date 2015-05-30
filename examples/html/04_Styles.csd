<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr                              =                       48000
ksmps                           =                       20
nchnls                          =                       2
iampdbfs                        init                    32768
                                prints                  "Default amplitude at 0 dBFS:  %9.4f\n", iampdbfs
idbafs                          init                    dbamp(iampdbfs)
                                prints                  "dbA at 0 dBFS:                 %9.4f\n", idbafs
iheadroom                       init                    6
                                prints                  "Headroom (dB):                 %9.4f\n", iheadroom
idbaheadroom                    init                    idbafs - iheadroom
                                prints                  "dbA at headroom:               %9.4f\n", idbaheadroom
iampheadroom                    init                    ampdb(idbaheadroom)
                                prints                  "Amplitude at headroom:        %9.4f\n", iampheadroom
                                prints                  "Balance so the overall amps at the end of performance -6 dbfs.\n"

                                alwayson                "Reverberation"
                                alwayson                "MasterOutput"

                                instr                   Reverberation
                                //////////////////////////////////////////////
                                // By Michael Gogins.
                                //////////////////////////////////////////////
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
if (gkReverberationEnabled == 0) then
aoutleft                        =                       ainleft
aoutright                       =                       ainright
kdry				            =			            1.0 - gkReverberationWet
else
awetleft, awetright             reverbsc                ainleft, ainright, gkReverberationDelay, 18000.0
aoutleft			            =			            ainleft *  kdry + awetleft  * gkReverberationWet
aoutright			            =			            ainright * kdry + awetright * gkReverberationWet
endif
                                outleta                 "outleft", aoutleft
                                outleta                 "outright", aoutright
                                endin


endin                           instr                   MasterOutput
                                //////////////////////////////////////////////
                                // By Michael Gogins.
                                //////////////////////////////////////////////
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
aoutleft                        =                       gkMasterLevel * ainleft
aoutright                       =                       gkMasterLevel * ainright
                                outs                    aoutleft, aoutright
                                endin




</CsInstruments>
<html>
<h1>Score Generator</h1>
</html>
<CsScore>
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
