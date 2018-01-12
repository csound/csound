#
# spec file for package csound
#
# Copyright (c) 2017 SUSE LINUX GmbH, Nuernberg, Germany.
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


%define support_fltk 0

Name:           csound
BuildRequires:  alsa-devel
BuildRequires:  bison
BuildRequires:  cmake
BuildRequires:  fdupes
BuildRequires:  flex
BuildRequires:  fluidsynth-devel
BuildRequires:  gcc-c++
BuildRequires:  jack-devel
BuildRequires:  liblo-devel
BuildRequires:  libsndfile-devel
BuildRequires:  portaudio-devel
BuildRequires:  portmidi-devel
BuildRequires:  python-devel
BuildRequires:  swig
%if %support_fltk
BuildRequires:  fltk-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libpng-devel
BuildRequires:  xorg-x11-devel
%endif
Summary:        Computer Sound Synthesis and Composition Program
License:        LGPL-2.1-or-later
Group:          Productivity/Multimedia/Sound/Utilities
Version:        6.10.0
Release:        0
Source:         csound-%{version}.tar.gz
Source1:        README.SUSE
Source2:        readme-csound6.txt
Source3:        CMakeLists.txt
Url:            http://www.csounds.com

BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Csound is a software synthesis program. It is fully modular and
supports an unlimited amount of oscillators and filters.  It includes
a language to specify sound (the orchestra language) and one to say
when to play a sound (the score language).  There is support for
multi-core synthesis, on-the-fly spectral transformations and a number
of analysis utilities and much more.  It is a descendent of the MUSIC
family.

%package devel
Summary:        Development files for Csound, a sound synthesis program
Group:          Development/Libraries/C and C++
Requires:       %{name} = %{version}
Provides:       %{name}-devel = %{version}

%description devel
Development files for Csound, mainly header files and some libraries.

%prep
%setup -q -n csound-%{version}
# remove __DATE__ from source files, causes unnecessary rebuilds
sed -i 's:__DATE__:"":' include/version.h
# copy readme
cp %{SOURCE1} .
cp %{SOURCE2} .
# replace CMakeLists.txt for now
rm CMakeLists.txt
cp %{SOURCE3} .
# fix encoding
iconv -f latin1 -t utf8 readme-csound6.txt > README

%build
%if %{_lib} == "lib64"
args="Word64=1"
%else
args=""
%endif
cmake -DBUILD_STATIC_LIBRARY=1 -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_ROOT%{_prefix} \
      -DUSE_LIB64=1 -DCMAKE_BUILD_TYPE=Release .
make prefix=%{_AKprefix} buildRelease=1 useDouble=1 useOSC=1 \
  buildVirtual=1 E_buildBeats=1 $args \
  customCCFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing" \
  customCXXFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"

%install
%if %{_lib} == "lib64"
args="--word64"
%else
args=""
%endif
mkdir -pv $RPM_BUILD_ROOT%{_datadir}/csound
make install
rm -f %{buildroot}%{_prefix}/csound6-*.md5sums %{buildroot}%{_bindir}/uninstall-csound6
rm -rf %{buildroot}%{_datadir}/doc/csound
# rename conflicting binary names
mv %{buildroot}%{_bindir}/sndinfo %{buildroot}%{_bindir}/csndinfo
mv %{buildroot}%{_bindir}/extract %{buildroot}%{_bindir}/csound-extract
rm -rf %{buildroot}/usr/share/csound

%fdupes -s %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog README README.SUSE
%{_bindir}/*
%{_libdir}/csound/
%{_libdir}/lib*.so*
/usr/share/locale

##%files translations

%files devel
%defattr(-,root,root)
%{_includedir}/csound/
%{_libdir}/libcsound64.a

%changelog
