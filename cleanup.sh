#!/bin/sh

if [ "$#" = "2" ] ; then
  if [ "$1" = "remove" ] ; then
    if ( test -e "$2" ) ; then
      echo "Removing '$2'"
      rm -Rf "$2" ;
    fi ;
    exit 0 ;
  fi ;
  exit -1 ;
elif [ "$#" != "0" ] ; then
  exit 0 ;
fi

# Remove static stuff first

"$0" remove ".sconf_temp"
"$0" remove "config.log"

"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.cc"
"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.h"
"$0" remove "./CsoundVST.py"
"$0" remove "./CsoundVST.pyc"
"$0" remove "./CsoundVST.pyo"
"$0" remove "./frontends/CsoundAC/CsoundAC_wrap.cc"
"$0" remove "./frontends/CsoundAC/CsoundAC_wrap.h"
"$0" remove "./CsoundAC.py"
"$0" remove "./CsoundAC.pyc"
"$0" remove "./CsoundAC.pyo"
"$0" remove "./interfaces/python_interface_wrap.cc"
"$0" remove "./interfaces/python_interface_wrap.h"
"$0" remove "./interfaces/java_interface_wrap.cc"
"$0" remove "./interfaces/java_interface_wrap.h"
"$0" remove "./interfaces/lua_interface_wrap.cc"
"$0" remove "./csnd.py"
"$0" remove "./csnd.pyc"
"$0" remove "./csnd.pyo"
"$0" remove "./interfaces/csnd.py"
"$0" remove "./interfaces/csnd.pyc"
"$0" remove "./interfaces/csnd.pyo"
"$0" remove "./csnd.jar"
"$0" remove "./interfaces/csnd"
"$0" remove "./Opcodes/Loris/scripting/loris_wrap.cc"
"$0" remove "./loris.py"
"$0" remove "./loris.pyc"
"$0" remove "./loris.pyo"
"$0" remove "./frontends/winsound/winsound.cxx"
"$0" remove "./frontends/winsound/winsound.h"
"$0" remove "./frontends/winsound/winsound.cpp"
"$0" remove "./frontends/CsoundVST/ScoreGeneratorVST_wrap.cc"
"$0" remove "./frontends/CsoundVST/ScoreGeneratorVST_wrap.h"
"$0" remove "./scoregen.py"
"$0" remove "./scoregen.pyc"
"$0" remove "./scoregen.pyo"
"$0" remove "./frontends/fltk_gui/CsoundAboutWindow_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundAboutWindow_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGlobalSettingsPanel_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGlobalSettingsPanel_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIConsole_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIConsole_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIMain_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundGUIMain_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundPerformanceSettingsPanel_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundPerformanceSettingsPanel_FLTK.hpp"
"$0" remove "./frontends/fltk_gui/CsoundUtilitiesWindow_FLTK.cpp"
"$0" remove "./frontends/fltk_gui/CsoundUtilitiesWindow_FLTK.hpp"
"$0" remove "./doc/html"

rm -f "./libcsound.so"
rm -f "./libcsound64.so"
rm -f "./_csnd.so"
rm -f "./_CsoundVST.so"
rm -f "./_CsoundAC.so"
rm -f "./libcsnd.so"


nocvs='-name CVS -prune ! -name CVS -o'

patterns='*.xmg *.o *.os *.so *.a *.wav *.aif* *.sf *.pcm *.exe *.dll
*.obj *.lib *.class *.mo'
findexpr='-iname .scons*'
for pat in $patterns; do
	findexpr="$findexpr -o -iname $pat"
done
findexpr="( $findexpr )"

find . $nocvs -type f $findexpr -exec "$0" remove '{}'  \;

find "./interfaces" $nocvs -type f -iname "*.java" -exec "$0" remove '{}' \;

notexecpatterns="*.cpp *.cxx *.h *.hpp *.fl *.i *.csd *.py *.sh *.sco *.orc
	*.txt *.tcli *.po *.html *.png *.gif"
notexecexpr="-iname *.c"
for pat in $notexecpatterns; do
	notexecexpr="$notexecexpr -o -iname $pat"
done
notexecexpr="( $notexecexpr )"

find . $nocvs -type f \! $notexecexpr -exec file '{}' \; | grep -E -e ':.*\<(ELF|80386|PE)\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove

