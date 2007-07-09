# Copyright none yet
# Distributed under the terms of the GNU General Public License v2
# $Header$

DESCRIPTION="Csound is a powerful and flexible system for digital sound synthesis and processing"

HOMEPAGE="http://csounds.com"

RESTRICT="nomirror"
MY_P=${P/csound-/Csound}
SRC_URI="mirror://sourceforge/${PN}/${MY_P}.tar.gz"

LICENSE="LGPL-2.1"
SLOT="0"
KEYWORDS="~amd64 ~x86"

IUSE=" alsa amd64 doc double dssi dynamic fltk jack java loris osc_opcodes pd portaudio portmidi ppc64 python tcltk vst"

RDEPEND="
	media-libs/ladspa-sdk
	media-libs/alsa-lib
	>=media-libs/libsndfile-1.0.12-r1"

DEPEND="${RDEPEND}
	>=x11-libs/fltk-1.1.0
	dev-util/scons
	=dev-lang/python-2.4*
	dev-lang/swig
	portaudio? ( =media-libs/portaudio-19* )
	python? ( >=dev-lang/python-2.4 )
	jack? ( media-sound/jack-audio-connection-kit )
	tcltk? ( dev-lang/tcl )
	tcltk? ( dev-lang/tk )
	java? ( virtual/jdk )
	vst? ( >=dev-libs/boost-1.32.1 )
	vst? ( dev-lang/swig )
	osc_opcodes? ( media-libs/liblo )
	pd? ( media-sound/pd )
	doc? ( app-doc/doxygen )
	dssi? ( >=media-libs/dssi-0.9.1 )"

S="${WORKDIR}/${MY_P}"

src_unpack() {
	unpack ${A}
	cd ${S}
	cp custom.py.mkg custom.py
}

src_compile() {
	local myconf="prefix=/usr"
	# Feed scons with use-enabled options
	! use alsa; myconf="${myconf} useALSA=$?"
	! use doc; myconf="${myconf} generatePdf=$?"
	! use double; myconf="${myconf} useDouble=$?"
	! use dssi; myconf="${myconf} buildDSSI=$?"
	! use dynamic; myconf="${myconf} dynamicCsoundLibrary=$?"
	! use fltk; myconf="${myconf} useFLTK=$? customCPPPATH=/usr/include/fltk-1.1 customLIBPATH=/usr/lib/fltk-1.1"
	! use jack; myconf="${myconf} useJack=$?"
	! use java; myconf="${myconf} buildJavaWrapper=$?"
	! use loris; myconf="${myconf} buildLoris=$?"
	! use osc_opcodes; myconf="${myconf} useOSC=$?"
	! use pd; myconf="${myconf} buildPDClass=$?"
	! use portaudio; myconf="${myconf} usePortAudio=$?"
	! use portmidi; myconf="${myconf} usePortMIDI=$?"
	! use python; myconf="${myconf} buildPythonOpcodes=$?"
	! use tcltk; myconf="${myconf} buildTclcsound=$?"
	! use vst; myconf="${myconf} buildCsoundVST=$?"
	( use amd64 || use ppc64 )  && myconf="${myconf} Word64=1"

	# No sneaking here ;)
	einfo "configuring with following options:"
	einfo "${myconf}"
	sleep 2

	# These addpredicts are to stop sandbox violation errors
	addpredict "/usr/include"
	addpredict "/usr/local/include"
	addpredict "/usr/lib"
	addpredict "/usr/local/lib"

	# now let's pray it worx
	scons ${myconf} || die "scons failed!"
}

src_install() {
	./install.py --prefix="/usr/" --instdir="${D}"
	cd ${D}/usr
	rm -f *.md5sums
}

pkg_postinst() {
	ewarn "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * "
	ewarn "This is a testing ebuild, a few build options are still missing."
	ewarn "Please report your success or lack thereof to the csound mailing list."
	ewarn "remember to set the OPCODEDIR variable to"
	ewarn "/usr/lib/csound/plugins or /usr/lib/csound/plugins64"
	ewarn "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * "
}

