Summary: Csound - sound synthesis language and library
Name: csound
%define version 5.08.91
Version: %version
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
BuildRoot: /home/victor/csoundxo/__package
Source: csound%version.tar.gz
# set this to match your Csound5 source directory above
%define csdir /home/victor/csoundxo/csound5

%description
Csound is a sound and music synthesis system, providing facilities for composition and performance over a wide range of platforms. It is not restricted to any style of music, having been used for many years in at least classical, pop, techno, ambient...

Authors:
--------
    Allan Lee
    Bill Gardner
    Bill Verplank
    Dan Ellis
    David Macintyre
    Eli Breder
    Gabriel Maldonado
    Greg Sullivan
    Hans Mikelson
    Istvan Varga
    Jean Piché
    John ffitch
    John Ramsdell
    Marc Resibois
    Mark Dolson
    Matt Ingalls
    Max Mathews
    Michael Casey
    Michael Clark
    Michael Gogins
    Mike Berry
    Paris Smaragdis
    Perry Cook
    Peter Neubäcker
    Peter Nix
    Rasmus Ekman
    Richard Dobson
    Richard Karpen
    Rob Shaw
    Robin Whittle
    Sean Costello
    Steven Yi
    Tom Erbe
    Victor Lazzarini
    Ville Pulkki

%prep
rm -Rf "$RPM_BUILD_ROOT"

%build
mkdir -p -m 0755 "$RPM_BUILD_ROOT"
cd "$RPM_BUILD_ROOT"
cp -aiR %{csdir}/custom.py %{csdir}/AUTHORS %{csdir}/COPYING %{csdir}/ChangeLog %{csdir}/Engine %{csdir}/H %{csdir}/INSTALL %{csdir}/InOut/ %{csdir}/OOps/ %{csdir}/Opcodes/ %{csdir}/SDIF/ .
cp -aiR %{csdir}/SConstruct %{csdir}/Top/  %{csdir}/frontends/ %{csdir}/interfaces/ %{csdir}/po/ %{csdir}/readme-csound5.txt %{csdir}/util .
tar cf "$RPM_SOURCE_DIR"/csound%{version}.tar ../csound5
gzip -f "$RPM_SOURCE_DIR"/csound%{version}.tar 
scons buildRelease=1 buildOLPC=1 gcc3opt=k6 useJack=0


%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
mkdir -p $RPM_BUILD_ROOT/usr/lib/csound
mkdir -p $RPM_BUILD_ROOT/usr/lib/csound/plugins
mkdir -p $RPM_BUILD_ROOT/usr/share
mkdir -p $RPM_BUILD_ROOT/usr/share/locale
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/en_GB
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/en_US
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/es_CO
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/fr
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/de
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/en_GB/LC_MESSAGES
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/en_US/LC_MESSAGES
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/es_CO/LC_MESSAGES
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/fr/LC_MESSAGES
mkdir -p $RPM_BUILD_ROOT/usr/share/locale/de/LC_MESSAGES
mkdir -p $RPM_BUILD_ROOT/usr/include
mkdir -p $RPM_BUILD_ROOT/usr/include/csound
mkdir -p $RPM_BUILD_ROOT/usr/share/doc
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/csound
mkdir -p $RPM_BUILD_ROOT/usr/lib/python2.5
mkdir -p $RPM_BUILD_ROOT/usr/lib/python2.5/site-packages
cd $RPM_BUILD_ROOT
mv H/cfgvar.h H/cscore.h H/csdl.h H/csound.h H/csound.hpp H/csoundCore.h H/cwindow.h H/msg_attr.h H/OpcodeBase.hpp H/pstream.h H/pvfileio.h H/soundio.h H/sysdep.h H/text.h H/version.h interfaces/CsoundFile.hpp interfaces/CppSound.hpp interfaces/filebuilding.h $RPM_BUILD_ROOT/usr/include/csound
mv ./csound $RPM_BUILD_ROOT/usr/bin
#mv ./sndinfo $RPM_BUILD_ROOT/usr/bin
#mv ./mixer $RPM_BUILD_ROOT/usr/bin
#mv ./scale $RPM_BUILD_ROOT/usr/bin
mv ./libcsound.so.5.1 ./libcsnd.so.5.1 $RPM_BUILD_ROOT/usr/lib
rm -rf ./libcsound.so ./libcsnd.so
mv ./lib*.so $RPM_BUILD_ROOT/usr/lib/csound/plugins
mv csnd.py _csnd.so $RPM_BUILD_ROOT/usr/lib/python2.5/site-packages
rm -rf $RPM_BUILD_ROOT/usr/lib/csound/plugins/libcsound*
rm -rf ./*5.1
mv ./po/en_GB/LC_MESSAGES/csound5.mo $RPM_BUILD_ROOT/usr/share/locale/en_GB/LC_MESSAGES
mv ./po/en_US/LC_MESSAGES/csound5.mo $RPM_BUILD_ROOT/usr/share/locale/en_US/LC_MESSAGES
mv ./po/es_CO/LC_MESSAGES/csound5.mo $RPM_BUILD_ROOT/usr/share/locale/es_CO/LC_MESSAGES
mv ./po/fr/LC_MESSAGES/csound5.mo $RPM_BUILD_ROOT/usr/share/locale/fr/LC_MESSAGES
mv ./po/de/LC_MESSAGES/csound5.mo $RPM_BUILD_ROOT/usr/share/locale/de/LC_MESSAGES
mv ./ChangeLog $RPM_BUILD_ROOT/usr/share/doc/csound/ChangeLog
mv ./COPYING $RPM_BUILD_ROOT/usr/share/doc/csound/COPYING
mv ./INSTALL $RPM_BUILD_ROOT/usr/share/doc/csound/INSTALL
mv ./readme-csound5.txt $RPM_BUILD_ROOT/usr/share/doc/csound/readme-csound5.txt
cp /usr/lib/liblo.so.0.6.0 $RPM_BUILD_ROOT/usr/lib
rm -rf AUTHORS COPYING ChangeLog Engine/ H/ INSTALL InOut/ OOps/ Opcodes/ SConstruct Top/ frontends/ interfaces/ po/ readme-csound5.txt util
rm -rf .sconf_temp .sconsign.dblite config.log custom.py envext SDIF

%clean
rm -rf $RPM_BUILD_ROOT

%post
ln -sf /usr/lib/libcsound.so.5.1 /usr/lib/libcsound.so
ln -sf /usr/lib/libcsnd.so.5.1 /usr/lib/libcsnd.so
ln -sf /usr/lib/liblo.so.0.6.0 /usr/lib/liblo.so.0
ln -sf /usr/lib/liblo.so.0 /usr/lib/liblo.so
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-, root, root, 0755)
/usr/bin/csound
/usr/lib/csound/plugins/libampmidid.so
/usr/lib/csound/plugins/libbarmodel.so
/usr/lib/csound/plugins/libcompress.so
/usr/lib/csound/plugins/libcs_date.so
/usr/lib/csound/plugins/libcs_pan2.so
/usr/lib/csound/plugins/libcs_pvs_ops.so
/usr/lib/csound/plugins/libeqfil.so
/usr/lib/csound/plugins/libgabnew.so
/usr/lib/csound/plugins/libgrain4.so
/usr/lib/csound/plugins/libharmon.so
/usr/lib/csound/plugins/libhrtfnew.so
/usr/lib/csound/plugins/libimage.so
/usr/lib/csound/plugins/libloscilx.so
/usr/lib/csound/plugins/libminmax.so
/usr/lib/csound/plugins/libmixer.so
/usr/lib/csound/plugins/libmodal4.so
/usr/lib/csound/plugins/libmutexops.so
/usr/lib/csound/plugins/libogg.so
/usr/lib/csound/plugins/liboggplay.so
/usr/lib/csound/plugins/libosc.so
/usr/lib/csound/plugins/libpartikkel.so
/usr/lib/csound/plugins/libphisem.so
/usr/lib/csound/plugins/libphysmod.so
/usr/lib/csound/plugins/libpitch.so
/usr/lib/csound/plugins/libptrack.so
/usr/lib/csound/plugins/libpvoc.so
/usr/lib/csound/plugins/libpvsbuffer.so
/usr/lib/csound/plugins/libscansyn.so
/usr/lib/csound/plugins/libscoreline.so
/usr/lib/csound/plugins/libsfont.so
/usr/lib/csound/plugins/libshape.so
/usr/lib/csound/plugins/libstackops.so
/usr/lib/csound/plugins/libstdopcod.so
/usr/lib/csound/plugins/libstdutil.so
/usr/lib/csound/plugins/libsystem_call.so
/usr/lib/csound/plugins/libudprecv.so
/usr/lib/csound/plugins/libudpsend.so
/usr/lib/csound/plugins/libugakbari.so
/usr/lib/csound/plugins/libvaops.so
/usr/lib/csound/plugins/libvosim.so
/usr/lib/csound/plugins/librtalsa.so
/usr/lib/libcsound.so.5.1
/usr/lib/libcsnd.so.5.1
/usr/lib/liblo.so.0.6.0
/usr/lib/python2.5/site-packages/_csnd.so
/usr/lib/python2.5/site-packages/csnd.py
/usr/lib/python2.5/site-packages/csnd.pyc
/usr/lib/python2.5/site-packages/csnd.pyo
/usr/share/doc/csound/ChangeLog
/usr/share/doc/csound/COPYING
/usr/share/doc/csound/INSTALL
/usr/share/doc/csound/readme-csound5.txt
/usr/share/locale/en_GB/LC_MESSAGES/csound5.mo
/usr/share/locale/en_US/LC_MESSAGES/csound5.mo
/usr/share/locale/es_CO/LC_MESSAGES/csound5.mo
/usr/share/locale/fr/LC_MESSAGES/csound5.mo
/usr/share/locale/de/LC_MESSAGES/csound5.mo
/usr/include/csound/cfgvar.h
/usr/include/csound/cscore.h
/usr/include/csound/csdl.h
/usr/include/csound/csound.h
/usr/include/csound/csound.hpp
/usr/include/csound/csoundCore.h
/usr/include/csound/cwindow.h
/usr/include/csound/msg_attr.h
/usr/include/csound/OpcodeBase.hpp
/usr/include/csound/pstream.h
/usr/include/csound/pvfileio.h
/usr/include/csound/soundio.h
/usr/include/csound/sysdep.h
/usr/include/csound/text.h
/usr/include/csound/version.h 
/usr/include/csound/CsoundFile.hpp 
/usr/include/csound/CppSound.hpp 
/usr/include/csound/filebuilding.h
