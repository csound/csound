Summary: Csound-dev - headers for Csound library
Name:   csound-dev       
%define version 5.08.91
Version: %version
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
Source: csound-%version.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:  python # ?
Requires: liblo libsndfile csound-%{version}     

%description
Csound C/C++ headers fo accessing the API and the interfaces library

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
cd %{_builddir}
rm -rf csound5
/bin/gzip -dc %{_sourcedir}/csound-%{version}.tar.gz | tar -xf -

%build

%install
rm -rf $RPM_BUILD_ROOT
cd %{_builddir}/csound5
python install-olpc.py --install-headers --instdir=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%defattr(-,root,root,-)
%{_includedir}/*

%changelog

* Tue Apr 01 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - first version of this spec

