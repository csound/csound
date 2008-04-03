Summary: Csound - sound synthesis language and library, OLPC subset
Name:   olpcsound        
Version: 5.08.91
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
Source: http://downloads.sourceforge.net/csound/olpcsound-%version.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: python scons alsa-lib-devel liblo-devel libsndfile-devel 
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
/usr/bin/scons buildOLPC=1

%install
rm -rf %{buildroot}
/usr/bin/python install-olpc.py --instdir=%{buildroot}
/usr/bin/python install-olpc.py --install-python --instdir=%{buildroot}
%find_lang csound5

%clean
rm -rf %{buildroot}

%post
ln -sf %{_libdir}/libcsound.so.5.1  %{_libdir}/libcsound.so
ln -sf %{_libdir}/libcsnd.so.5.1  %{_libdir}/libcsnd.so
/sbin/ldconfig

%postun
rm -f %{_libdir}/libcsound.so
rm -f %{_libdir}/libcsnd.so
/sbin/ldconfig

%files -f %{_builddir}/%{name}-%{version}/csound5.lang
%defattr(-,root,root,-)
%{_bindir}/*
%{_libdir}/csound/plugins/*
%{_libdir}/lib*
%{python_site_dir}/*
%{_datadir}/doc/csound/*


%changelog

* Wed Apr 02 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - initial version of this spec

