Summary: Csound - sound synthesis language and library
Name:   csound        
%define version 5.08.91
Version: %version
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
Source: csound-%version.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:  
Requires: liblo libsndfile      

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
%setup -q

%build
scons buildOLPC=1

%install
rm -rf $RPM_BUILD_ROOT
install-olpc.py --instdir=$RPM_BUILD_ROOT



%clean
rm -rf $RPM_BUILD_ROOT

%post
ln -sf /usr/lib/libcsound.so.5.1 /usr/lib/libcsound.so
ln -sf /usr/lib/libcsnd.so.5.1 /usr/lib/libcsnd.so
/sbin/ldconfig

%postun
rm -f /usr/lib/libcsound.so


%files
%defattr(-,root,root,-)
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

%changelog

* 31-03-08 Victor Lazzarini <vlazzarini@nuim.ie>
 - initial version of this spec
