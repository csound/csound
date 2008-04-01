Summary: Csound-python - python bindings for Csound
Name:   csound-python        
%define version 5.08.91
Version: %version
Release: 0
URL: http://csound.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
Source: csound-%version.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires: python scons # ?
Requires: liblo libsndfile csound-%{version}     
%define python_site_dir /usr/lib/python2.5/site-packages

%description
Csound Python bindings allow Python scripts to access the
Csound API

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
cd %{_builddir}/csound5
scons buildOLPC=1

%install
rm -rf $RPM_BUILD_ROOT
cd %{_builddir}/csound5
python install-olpc.py --install-python --instdir=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun

%files
%defattr(-,root,root,-)
%{python_site_dir}/*

%changelog

* Tue Apr 01 2008  Victor Lazzarini <vlazzarini@nuim.ie>
 - first version of this spec

