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
%define python_site_dir /usr/lib/python2.5/site-packages

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
%{_libdir}/csound/plugins/*
%{_libdir}/lib*
%{python_site_dir}/*
%{_datadir}/doc/csound/*
%{_datadir}/locale/en_GB/LC_MESSAGES/csound5.mo
%{_datadir}/locale/en_US/LC_MESSAGES/csound5.mo
%{_datadir}/locale/es_CO/LC_MESSAGES/csound5.mo
%{_datadir}/locale/fr/LC_MESSAGES/csound5.mo
%{_datadir}/locale/de/LC_MESSAGES/csound5.mo
%{_includedir}/csound/*

%changelog

* Tue Apr 01 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - simplified the filelist in this spec
* Mon Mar 31 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - initial version of this spec

