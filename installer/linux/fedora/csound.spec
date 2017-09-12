%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(1))")}
%{!?python_version: %global python_version %(%{__python} -c "import sys; print '%s.%s' % sys.version_info[:2]")}

# Csound is really dumb about 64-bit
%ifarch x86_64 ia64 ppc64 sparc64 s390x
%define build64bit 1
%define install64bit --word64
%define useDouble 1
%else
%define build64bit 0
%define install64bit %{nil}
%define useDouble 0
%endif

Summary:       A sound synthesis language and library
Name:          csound
Version:       5.16.1
Release:       5%{?dist}
URL:           http://csound.sourceforge.net/
License:       LGPLv2+
Group:         Applications/Multimedia

BuildRequires: swig scons libsndfile-devel libpng-devel libjpeg-devel
BuildRequires: python python-devel
BuildRequires: flex bison
BuildRequires: alsa-lib-devel jack-audio-connection-kit-devel pulseaudio-libs-devel
BuildRequires: fluidsynth-devel liblo-devel dssi-devel lua-devel
BuildRequires: fltk-devel fltk-fluid
BuildRequires: java-devel >= 1.4.0
BuildRequires: jpackage-utils >= 1.5
BuildRequires: java-gcj-compat-devel
BuildRequires: tk-devel tcl-devel
BuildRequires: tetex tetex-latex libxslt
BuildRequires: libvorbis-devel libogg-devel
BuildRequires: gettext
BuildRequires: gcc-c++ boost-devel

Source0: http://downloads.sourceforge.net/csound/Csound%{version}.tar.gz
Source1: http://downloads.sourceforge.net/csound/Csound5.16-manual-src.tar.gz
Source2: http://downloads.sourceforge.net/csound/Csound5.16_manual_html.zip

Patch0: csound-5.12.1-fixpython.patch
Patch1: csound-5.13.0-no-usr-local.patch
Patch2: csound-5.13.0-default-opcodedir.patch
Patch3: csound-5.10.1-rtalsa-fix.patch
Patch4: csound-5.13.0-tclversion.patch
Patch5: csound-5.10.1-64-bit-plugin-path.patch
Patch6: csound-5.13.0-fix-conflicts.patch
Patch7: csound-5.13.0-fix-locale-install.patch
Patch8: csound-5.10.1-enable-oggplay.patch
Patch9: csound-5.13.0-default-pulse.patch
Patch10: csound-5.13.0-compile-flag.patch
Patch11: csound-5.13.0-fixpythonint.patch
Patch12: csound-5.13.0-fltk.patch

%description
Csound is a sound and music synthesis system, providing facilities for
composition and performance over a wide range of platforms. It is not
restricted to any style of music, having been used for many years in
at least classical, pop, techno, ambient...

%package devel
Summary: Csound development files and libraries
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
Contains headers and libraries for developing applications that use Csound.

%package python
Summary: Python Csound development files and libraries
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: python

%description python
Contains Python language bindings for developing Python applications that
use Csound.

%package python-devel
Summary: Csound python development files and libraries
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description python-devel
Contains libraries for developing against csound-python.

%package java
Summary: Java Csound support
Group: System Environment/Libraries
Requires: %{name} = %{version}-%{release}
Requires:         jpackage-utils >= 1.5
Requires:         java-1.5.0-gcj
Requires(post):   jpackage-utils >= 1.5
Requires(postun): jpackage-utils >= 1.5
Requires(post):   java-gcj-compat
Requires(postun): java-gcj-compat
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description java
Contains Java language bindings for developing and running Java
applications that use Csound.

%package javadoc
Summary: API documentation for Java Csound support
Group: Documentation

%description javadoc
API documentation for the %{name}-java package.

%package tk
Summary: Tcl/Tk related Csound utilities
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: tcl tk

%description tk
Contains Tcl/Tk related Csound utilities

%package gui
Summary: A FLTK-based GUI for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: fltk xdg-utils

%description gui
Contains a FLTK-based GUI for Csound

%package fltk
Summary: FLTK plugins for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: fltk

%description fltk
Contains FLTK plugins for csound

%package jack
Summary: Jack Audio plugins for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: jack-audio-connection-kit

%description jack
Contains Jack Audio plugins for Csound

%package fluidsynth
Summary: Fluidsyth soundfont plugin for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}

%description fluidsynth
Contains Fluidsynth soundfont plugin for Csound.

%package dssi
Summary: Disposable Soft Synth Interface (DSSI) plugin for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: dssi

%description dssi
Disposable Soft Synth Interface (DSSI) plugin for Csound

%package osc
Summary: Open Sound Control (OSC) plugin for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}

%description osc
Open Sound Control (OSC) plugin for Csound

%package virtual-keyboard
Summary: Virtual MIDI keyboard plugin for Csound
Group: Applications/Multimedia
Requires: %{name} = %{version}-%{release}
Requires: fltk

%description virtual-keyboard
A virtual MIDI keyboard plugin for Csound

%package manual
Summary: Csound manual
Group: Documentation
Requires: %{name} = %{version}-%{release}
BuildArch: noarch

%description manual
Canonical Reference Manual for Csound.


%prep
%setup -q -n Csound%{version}
%patch0 -p1 -b .fixpython
%patch1 -p1 -b .no-usr-local
%patch2 -p1 -b .default-opcodedir
%patch3 -p1 -b .rtalsa
%patch4 -p1 -b .makebuild
%patch5 -p1 -b .64-bit-plugin-path
%patch6 -p1 -b .fix-conflicts
%patch7 -p1 -b .fix-local-install
%patch8 -p1 -b .enable-oggplay
%patch9 -p1 -b .default-pulse
%patch10 -p1 -b .compile-flag
%patch11 -p1 -b .fixpythonint
%patch12 -p1 -b .fltk

tar xf %{SOURCE1}
(cd manual; unzip -q %{SOURCE2})

%build

# Adjust location of the documentation for the GUI bits
sed -ie 's#\"firefox /usr/local/share/doc/csound/manual/#\"xdg-open file://%{_docdir}/%{name}-manual-%{version}/#' 

scons dynamicCsoundLibrary=1 \
      buildRelease=1 \
      noDebug=0 \
      disableGStabs=1 \
      buildInterfaces=1 \
      useGettext=1 \
      useALSA=1 \
      usePortAudio=0 \
      usePortMIDI=0 \
      useOGG=1 \
      useOSC=1 \
      useJack=1 \
      useFLTK=1 \
      buildVirtual=1 \
      useFluidsynth=1 \
      generatePdf=0 \
      buildCsound5GUI=1 \
      pythonVersion=%{python_version} \
      buildPythonOpcodes=1 \
      buildPythonWrapper=1 \
      buildLuaWrapper=1 \
      buildTclcsound=1 \
      buildJavaWrapper=1 \
      buildDSSI=1 \
      buildUtilities=1 \
      prefix=%{_prefix} \
      customCCFLAGS="%{optflags}" \
      customCXXFLAGS="%{optflags}" \
      Word64=%{build64bit} \
      useDouble=%{useDouble}

# Generate javadoc
(cd interfaces; javadoc *.java)

%install
%{__rm} -rf %{buildroot}
%{__python} install.py --prefix=%{_prefix} --instdir=%{buildroot} %{install64bit}
%{__rm} -f %{buildroot}%{_docdir}/%{name}/COPYING
%{__rm} -f %{buildroot}%{_docdir}/%{name}/ChangeLog
%{__rm} -f %{buildroot}%{_docdir}/%{name}/INSTALL
%{__rm} -f %{buildroot}%{_docdir}/%{name}/readme-csound5.txt
%{__rm} -f %{buildroot}%{_bindir}/uninstall-csound5
%{__rm} -f %{buildroot}%{_prefix}/csound5-*.md5sums

install -dm 755 %{buildroot}%{_javadir}
(cd %{buildroot}%{_javadir}; ln -s %{_libdir}/%{name}/java/csnd.jar .)

install -dm 644 %{buildroot}%{_javadocdir}/%{name}-java
%{__chmod} -R 755 %{buildroot}%{_javadocdir}/%{name}-java
(cd interfaces; tar cf - *.html csnd/*.html) | (cd %{buildroot}%{_javadocdir}/%{name}-java; tar xvf -)

%{_bindir}/aot-compile-rpm

%find_lang %{name}5

%clean
%{__rm} -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post java
if [ -x %{_bindir}/rebuild-gcj-db ]; then
  %{_bindir}/rebuild-gcj-db
fi

%postun java
if [ -x %{_bindir}/rebuild-gcj-db ]; then
  %{_bindir}/rebuild-gcj-db
fi

%files -f %{name}5.lang
%defattr(-,root,root,-)
%doc COPYING ChangeLog readme-csound5.txt
%{_bindir}/atsa
%{_bindir}/cs-launcher
%{_bindir}/csb64enc
%{_bindir}/csound
%{_bindir}/cvanal
%{_bindir}/dnoise
%{_bindir}/cs-envext
%{_bindir}/cs-extract
%{_bindir}/cs-extractor
%{_bindir}/het_export
%{_bindir}/het_import
%{_bindir}/hetro
%{_bindir}/lpanal
%{_bindir}/lpc_export
%{_bindir}/lpc_import
%{_bindir}/makecsd
%{_bindir}/cs-mixer
%{_bindir}/pvanal
%{_bindir}/pvlook
%{_bindir}/cs-scale
%{_bindir}/cs-scot
%{_bindir}/scsort
%{_bindir}/cs-sndinfo
%{_bindir}/cs-srconv
%{_bindir}/pv_export
%{_bindir}/pv_import
%{_libdir}/lib%{name}.so.5.2
%dir %{_libdir}/%{name}/plugins
%{_libdir}/%{name}/plugins/libambicode1.so
%{_libdir}/%{name}/plugins/libampmidid.so
%{_libdir}/%{name}/plugins/libbabo.so
%{_libdir}/%{name}/plugins/libbarmodel.so
%{_libdir}/%{name}/plugins/libcompress.so
%{_libdir}/%{name}/plugins/libcontrol.so
%{_libdir}/%{name}/plugins/libchua.so
%{_libdir}/%{name}/plugins/libcrossfm.so
%{_libdir}/%{name}/plugins/libcs_date.so
%{_libdir}/%{name}/plugins/libcs_pan2.so
%{_libdir}/%{name}/plugins/libcs_pvs_ops.so
%{_libdir}/%{name}/plugins/libdoppler.so
%{_libdir}/%{name}/plugins/libeqfil.so
%{_libdir}/%{name}/plugins/libftest.so
%{_libdir}/%{name}/plugins/libgabnew.so
%{_libdir}/%{name}/plugins/libgrain4.so
%{_libdir}/%{name}/plugins/libharmon.so
%{_libdir}/%{name}/plugins/libhrtferX.so
%{_libdir}/%{name}/plugins/libhrtfnew.so
%{_libdir}/%{name}/plugins/libimage.so
%{_libdir}/%{name}/plugins/libjacko.so
%{_libdir}/%{name}/plugins/libloscilx.so
%{_libdir}/%{name}/plugins/libminmax.so
%{_libdir}/%{name}/plugins/libmixer.so
%{_libdir}/%{name}/plugins/libmodal4.so
%{_libdir}/%{name}/plugins/libmodmatrix.so
%{_libdir}/%{name}/plugins/libmutexops.so
%{_libdir}/%{name}/plugins/liboggplay.so
%{_libdir}/%{name}/plugins/libpartikkel.so
%{_libdir}/%{name}/plugins/libphisem.so
%{_libdir}/%{name}/plugins/libphysmod.so
%{_libdir}/%{name}/plugins/libpitch.so
%{_libdir}/%{name}/plugins/libptrack.so
%{_libdir}/%{name}/plugins/libpvlock.so
%{_libdir}/%{name}/plugins/libpvoc.so
%{_libdir}/%{name}/plugins/libpvsbuffer.so
%{_libdir}/%{name}/plugins/libpy.so
%{_libdir}/%{name}/plugins/librtalsa.so
%{_libdir}/%{name}/plugins/librtpulse.so
%{_libdir}/%{name}/plugins/libscansyn.so
%{_libdir}/%{name}/plugins/libscoreline.so
%{_libdir}/%{name}/plugins/libsfont.so
%{_libdir}/%{name}/plugins/libshape.so
%{_libdir}/%{name}/plugins/libsignalflowgraph.so
%{_libdir}/%{name}/plugins/libstackops.so
%{_libdir}/%{name}/plugins/libstdopcod.so
%{_libdir}/%{name}/plugins/libstdutil.so
%{_libdir}/%{name}/plugins/libsystem_call.so
%{_libdir}/%{name}/plugins/libtabsum.so
%{_libdir}/%{name}/plugins/libudprecv.so
%{_libdir}/%{name}/plugins/libudpsend.so
%{_libdir}/%{name}/plugins/libugakbari.so
%{_libdir}/%{name}/plugins/liburandom.so
%{_libdir}/%{name}/plugins/libvaops.so
%{_libdir}/%{name}/plugins/libvbap.so
%{_libdir}/%{name}/plugins/libvosim.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/
%{_libdir}/lib%{name}.so
%{_libdir}/libcsnd.so

%files python
%defattr(-,root,root,-)
%{_libdir}/libcsnd.so.5.2
%{python_sitearch}/_csnd*
%{python_sitearch}/csnd*

%files python-devel
%defattr(-,root,root,-)
# %{_libdir}/libcsnd.so

%files java
%defattr(-,root,root,-)
%{_libdir}/lib_jcsound.so
%{_libdir}/%{name}/java/
%{_javadir}/csnd.jar
%attr(-,root,root) %{_libdir}/gcj/%{name}

%files javadoc
%defattr(-,root,root,-)
%doc COPYING
%doc %{_javadocdir}/%{name}-java

%files tk
%defattr(-,root,root,-)
%{_libdir}/%{name}/tcl/
%{_bindir}/matrix.tk
%{_bindir}/brkpt
%{_bindir}/linseg
%{_bindir}/tabdes
%{_bindir}/cstclsh
%{_bindir}/cswish

%files gui
%defattr(-,root,root,-)
%{_bindir}/csound5gui

%files fltk
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/libwidgets.so

%files jack
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/librtjack.so
%{_libdir}/%{name}/plugins/libjackTransport.so

%files fluidsynth
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/libfluidOpcodes.so

%files dssi
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/libdssi4cs.so

%files osc
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/libosc.so

%files virtual-keyboard
%defattr(-,root,root,-)
%{_libdir}/%{name}/plugins/libvirtual.so

%files manual
%defattr(-,root,root,-)
%doc manual/copying.txt manual/credits.txt manual/readme.txt manual/news.txt
%doc manual/html/*
%doc manual/examples/*

%changelog
* Tue Jun 14 2011 Peter Robinson <pbrobinson@gmail.com> - 5.13.0-5
- Build the old Parser as the new Parser isn't stable even though it default!

* Wed Jun 01 2011 Ralf Corsépius <corsepiu@fedoraproject.org> - 5.13.0-4
- Reflect fltk include paths having changed.

* Fri May 27 2011 Peter Robinson <pbrobinson@gmail.com> - 5.13.0-3
- Bump build for new fltk

* Fri Apr 29 2011 Dan Horák <dan[at]danny.cz> - 5.13.0-2
- mark s390x as 64-bit arch

* Wed Apr  6 2011 Peter Robinson <pbrobinson@gmail.com> - 5.13.0-1
- Update to 5.13.0.

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.12.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sun Dec 26 2010 Peter Robinson <pbrobinson@gmail.com> - 5.12.1-1
- Update to 5.12.1.

* Sat Jul 31 2010 Toshio Kuratomi <toshio@fedoraproject.org> - 5.10.1-21
- Fix python location

* Wed Jul 21 2010 David Malcolm <dmalcolm@redhat.com> - 5.10.1-20
- Rebuilt for https://fedoraproject.org/wiki/Features/Python_2.7/MassRebuild

* Tue Jul 20 2010 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-19
- bump build

* Mon Jul 12 2010 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-18
- Add license file to -javadocs

* Sat Jan  9 2010 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-17
- Some further cleanups

* Sat Jan  9 2010 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-16
- Some further cleanups

* Sat Jan  9 2010 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-15
- Updated to the new python sysarch spec file reqs

* Thu Dec  3 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-14
- Updated python patch thanks to dsd.

* Tue Oct 20 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-13
- Fix use of multiple midi devices, fix segfault (RHBZ 529293)

* Sat Sep  5 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-12
- Build fixes, set PulseAudio as default

* Tue Aug 18 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-11
- Further python build fixes

* Fri Jul 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.10.1-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Jul 16 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-9
- Update included files

* Thu Jul 16 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-8
- Apply patch to fix libcsnd.so

* Thu Jun 18 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-7
- Obsolete olpcsound

* Thu May 28 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-6
- Obsolete csound-tutorial

* Tue May 12 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-4
- Once more with feeling :-)

* Tue May 12 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-3
- Some further spec fixes

* Tue May 12 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-2
- Some build fixes. Enable pulseaudio support

* Mon May 11 2009 Peter Robinson <pbrobinson@gmail.com> - 5.10.1-1
- Update to 5.10.1 based massively on dcbw's 5.07 spec from the OLPC-2 cvs branch
  rebase what looks to be relevant pataches from both branches
  add a number of other build fixes

* Tue Feb 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.03.0-21
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sun Dec 28 2008 Dennis Gilmore <dennis@ausil.us> - 5.03.0-20
- add sparc64 to list of 64 bit arches

* Sat Nov 29 2008 Ignacio Vazquez-Abrams <ivazqueznet+rpm@gmail.com> - 5.03.0-19
- Fix locations for Python 2.6

* Sat Nov 29 2008 Ignacio Vazquez-Abrams <ivazqueznet+rpm@gmail.com> - 5.03.0-18
- Rebuild for Python 2.6

* Thu May 22 2008 Seth Vidal <skvidal at fedoraproject.org> - 5.03.0-17
- license tag fix

* Fri Feb  1 2008 Dan Williams <dcbw@redhat.com> - 5.03.0-16
- Fix default plugin path on 64-bit platforms (rh #407911)
- Fix file conflicts with other packages (rh #210215)
- Fix unowned directories (rh #233830)

* Thu Jan 10 2008 Caolan McNamara <caolanm@redhat.com> - 5.03.0-15
- Resolves: rhbz#428176 make build

* Thu Jan 03 2008 Alex Lancaster <alexlan at fedoraproject dot org> - 5.03.0-14
- Rebuild for new tcl (8.5)

* Mon Apr  2 2007 Thomas Fitzsimmons <fitzsim@redhat.com> - 5.03.0-13
- Patch out FLTK widget initialization code made unnecessary by
  fltk-fluid 1.1.8, snapshot r5555
- Update python site-packages version to 2.5

* Sat Mar 31 2007 Thomas Fitzsimmons <fitzsim@redhat.com> - 5.03.0-12
- Require java-1.5.0-gcj

* Tue Feb 20 2007 Dan Williams <dcbw@redhat.com> - 5.03.0-11
- Rebuild for Python 2.5 (again)
- Fix rtalsa compile error (RH #220856)

* Tue Feb 20 2007 Dan Williams <dcbw@redhat.com> - 5.03.0-10
- Rebuild for Python 2.5

* Wed Nov  8 2006 Dan Williams <dcbw@redhat.com> 5.03.0-9
- CVS snapshot for fixes to the virtual midi keyboard

* Wed Nov  1 2006 Dan Williams <dcbw@redhat.com> 5.03.0-8
- CVS snapshot to pick up virtual Midi keyboard
- Make the default for OPCODEDIR be where the plugins are actually installed

* Sat Oct 28 2006 Dan Williams <dcbw@redhat.com> 5.03.0-7
- Rebuild to drop old source tarball

* Fri Oct 27 2006 Dan Williams <dcbw@redhat.com> 5.03.0-6
- Update to a cvs snapshot for the remote plugin and a few other fixes
- Split csound FLTK plugin out from -gui package since it's unrelated to the GUI bits
- Put the virtual MIDI keyboard into its own package

* Wed Oct 25 2006 Dan Williams <dcbw@redhat.com> 5.03.0-5
- Fix the remote plugin's local IP address read code, add more error checking

* Mon Oct 23 2006 Dan Williams <dcbw@redhat.com> 5.03.0-4
- Drop csound-5.03.0-uninitialized.patch, upstream
- Drop csound-5.03.0-printf-redef.patch, upstream
- CVS snapshot to grab some updated opcodes and fixes
- Make disabling -gstabs an option for better upstream palatability
- Disable atsa; it breaks the build for some unknown reason

* Fri Sep  8 2006 Dan Williams <dcbw@redhat.com> 5.03.0-3
- csound-5.03.0-no-gstabs.patch added; produce dwarf2 like everyone else

* Fri Aug 24 2006 Dan Williams <dcbw@redhat.com> 5.03.0-2
- Kill printf redefinition
- Remove zero-length ifthen.csd
- Remove explicit liblo dep in csound-osc
- Add fluidsynth plugin

* Fri Aug 24 2006 Dan Williams <dcbw@redhat.com> 5.03.0-1
- Package for Fedora Extras
