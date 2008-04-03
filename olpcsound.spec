Summary: Csound - sound synthesis language and library, OLPC subset
Name:   olpcsound        
Version: 5.08.91
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
Source: http://downloads.sourceforge.net/csound/olpcsound-%version.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: swig python scons alsa-lib-devel liblo-devel libsndfile-devel 
BuildRequires: libpng-devel libjpeg-devel libvorbis-devel libogg-devel gettext python-devel
Requires: liblo libsndfile      
%define python_site_dir %{_libdir}/python2.5/site-packages

%description
olpcsound is a subset of the Csound sound and music synthesis system, tailored specifically for  XO platform. It provides facilities for composition and performance over a wide range of platforms. It is not restricted to any style of music, having been used for many years in at least classical, pop, techno, ambient... 

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
    Jean Pich
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

%package devel
Summary: Csound development files and libraries
Group: Development/Libraries
Requires: %{name}=%{version}-%{release}

%description devel
Headers and libraries for Csound-based application development

%prep
%setup -q

%build
/usr/bin/scons buildOLPC=1

%install
%{__rm} -rf %{buildroot}
%{__python} install-olpc.py --install-python --install-headers --instdir=%{buildroot}
%find_lang csound5

%clean
%{__rm} -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f %{_builddir}/%{name}-%{version}/csound5.lang
%defattr(-,root,root,-)
%{_bindir}/*
%{_libdir}/csound/plugins/*
%{_libdir}/libcsound.so.5.1
%{_libdir}/libcsnd.so.5.1
%{python_site_dir}/*
%{_datadir}/doc/csound/*

%files devel
%defattr(-,root,root,0755)
%{_includedir}/csound/*
%{_libdir}/libcsound.so
%{_libdir}/libcsnd.so


%changelog

* Wed Apr 02 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - initial version of this spec

