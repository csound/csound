export IFMKBASE=../csound5_install/csoundlib/package_contents/Library/Frameworks
export APPSBASE=../csound5_install/csoundapps/package_contents/usr/local/bin
export FMKBASE=./CsoundLib.framework

rm -R $IFMKBASE/CsoundLib.framework
cp _csnd.so ../csound5_install/csoundlib/package_contents/System/Library/Frameworks/Python.framework/Versions/2.3/lib/Python2.3/
cp csnd.py ../csound5_install/csoundlib/package_contents/System/Library/Frameworks/Python.framework/Versions/2.3/lib/Python2.3/
cp lib_csnd.dylib $FMKBASE/Versions/5.1/
cp tclcsound.dylib $FMKBASE/Versions/5.1/Resources/TclTk/
cp csladspa.so  $FMKBASE/Versions/5.1/Resources/csladspa/csladspa.so
cp lib_jcsound.jnilib $FMKBASE/Versions/5.1/Resources/Java/
cp csnd.jar $FMKBASE/Versions/5.1/Resources/Java/
cp csoundapi~.pd_darwin $FMKBASE/Versions/5.1/Resources/PD/
cp examples/csoundapi_tilde/csoundapi-osx.pd $FMKBASE/Versions/5.1/Resources/PD/csoundapi.pd
cp examples/csoundapi_tilde/csapi_demo.csd ../csound5_install/csoundlib/package_contents/Library/Documentation/
cp -R ../manual/html $FMKBASE/Resources
rm -R $FMKBASE/Resources/Manual
mv $FMKBASE/Resources/html $FMKBASE/Resources/Manual
cp interfaces/*.hpp $FMKBASE/Headers
cp csladspa.so ../csound5_install/csoundApps/package_contents/Library/Audio/Plug-Ins/LADSPA/csladspa.so
cp -R $FMKBASE $IFMKBASE
          
cp csound  $APPSBASE
cp dnoise   $APPSBASE
cp het_export $APPSBASE     
cp lpanal  $APPSBASE        
cp scale  $APPSBASE         
cp tabdes $APPSBASE
cp cs   $APPSBASE           
cp cstclsh $APPSBASE        
cp envext $APPSBASE          
cp het_import $APPSBASE     
cp lpc_export $APPSBASE     
cp mixer  $APPSBASE         
cp scsort $APPSBASE
cp cswish  $APPSBASE        
cp extract $APPSBASE         
cp hetro $APPSBASE          
cp lpc_import $APPSBASE     
cp pvanal  $APPSBASE        
cp sndinfo $APPSBASE
cp csb64enc $APPSBASE        
cp cvanal  $APPSBASE        
cp extractor $APPSBASE       
cp linseg    $APPSBASE      
cp makecsd   $APPSBASE      
cp pvlook   $APPSBASE       
cp srconv $APPSBASE
cp atsa  $APPSBASE

cp cswish frontends/tclcsound/cswish.app/Contents/MacOS/.
cp -R frontends/tclcsound/cswish.app "../csound5_install/csoundapps/package_contents/Applications/Csound 5 Wish.app"
cp -R "frontends/OSX/build/Deployment/Csound 5.app" ../csound5_install/csoundapps/package_contents/Applications
cp -R frontends/Winsound/Winsound.app "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/."
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/Resources/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/Winsound.app/Contents/MacOS/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/Resources/CVS"
rm -R "../csound5_install/csoundapps/package_contents/Applications/Csound 5 frontends/csound5GUI.app/Contents/MacOS/CVS"


