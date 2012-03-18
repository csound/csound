#
# spec file for package csound
#
# Copyright (c) 2011 SUSE LINUX Products GmbH, Nuernberg, Germany.
#           (c) 2012 Csound Developers
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#



Name:           csound
%define support_fltk	0
BuildRequires:  alsa-devel fdupes fluidsynth-devel gcc-c++ jack-devel liblo-devel portaudio-devel python-devel scons swig
%if %support_fltk
BuildRequires:  fltk-devel libjpeg-devel libpng-devel xorg-x11-devel
%endif
Summary:        Computer Sound Synthesis and Composition Program
Version:        5.17.1
Release:        134
License:        GFDL-1.2 ; LGPL-2.1+ ; MIT
Group:          Productivity/Multimedia/Sound/Utilities
Source:         Csound%{version}.tar.bz2
Source1:        README.SuSE
Url:            http://www.csounds.com
AutoReq:        on
Autoprov:       off
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Csound is a software synthesis program. It is fully modular and
supports an unlimited amount of oscillators and filters.

For detailed information, refer to http://www.csounds.com.



%prep
%setup -q -n Csound%{version}
cp %{SOURCE1} .
# fix encoding
iconv -f latin1 -t utf8 readme-csound5.txt > readme-csound5.txt.utf8
mv readme-csound5.txt.utf8 readme-csound5.txt
test -f custom.py || cp custom.py.mkg custom.py

%build
%if %_lib == "lib64"
args="Word64=1 Lib64=1"
%else
args=""
%endif
scons prefix=%{_prefix} buildRelease=1 useDouble=1 useOSC=1 \
  buildVirtual=1 buildBeats=1 $args \
  customCCFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing" \
  customCXXFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"

%install
%if %_lib == "lib64"
args="--word64"
%else
args=""
%endif
mkdir -pv $RPM_BUILD_ROOT%{_datadir}/csound
./install.py --prefix=%{_prefix} --instdir="$RPM_BUILD_ROOT" $args
rm -f $RPM_BUILD_ROOT%{_prefix}/csound5-*.md5sums
rm -rf $RPM_BUILD_ROOT%{_datadir}/doc/csound
# rename conflicting binary names
mv $RPM_BUILD_ROOT%{_bindir}/sndinfo $RPM_BUILD_ROOT%{_bindir}/csndinfo
mv $RPM_BUILD_ROOT%{_bindir}/extract $RPM_BUILD_ROOT%{_bindir}/csound-extract
# remove devel files
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -rf $RPM_BUILD_ROOT%{_includedir}
%fdupes -s $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING ChangeLog INSTALL readme-csound5.txt README.SuSE
%{_bindir}/*
%{_libdir}/csound
%{_datadir}/csound
%{_datadir}/locale
# %{_includedir}/*
# %{_libdir}/lib*

%changelog
