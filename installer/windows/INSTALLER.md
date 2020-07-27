# CSOUND FOR WINDOWS

(Note: This document is out of date and should be revised or removed.)

Version 6.09 beta

[![Build Status](https://travis-ci.org/csound/csound.svg?branch=develop)](https://travis-ci.org/csound/csound)
![Coverity Status](https://scan.coverity.com/projects/1822/badge.svg)
[![Build status](https://ci.appveyor.com/api/projects/status/1qamc986774rsbjq/branch/develop?svg=true)](https://ci.appveyor.com/project/csound/csound/branch/develop)

Csound is a user-programmable and user-extensible sound processing language and software sound synthesizer. Csound contains arguably the largest collection of unit generators of any software sound synthesizer. Online resources include many example pieces, instrument definitions, and tutorials.

Csound is copyright (c) 1991 Barry Vercoe, John ffitch, and other contributors.

Csound is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

Csound is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this software; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# THE WINDOWS INSTALLER

The Windows installer for Csound comes with:

1. The Csound shared library, which contains Csound's core functionality.
2. A number of programming language interfaces to Csound:
    1. C and C++, along with header files and libraries for embedding and calling Csound.
    2. Python (both running Csound from Python, and running Python from Csound).
    3. Java.
    4. Common Lisp.
4. Command-line programs, including Csound itself and a variety of utility programs.
5. The CsoundQt editor for Csound, including the ability to define custom user interfaces in Csound, or even to run local Web pages that embed Csound and give Csound access to all the capabilities of HTML5 including JavaScript, WebGL, local data store, math typesetting, and on and on.
6. Csound.node, which embeds Csound in NW.js, with much the same results as running HTML5 code in CsoundQt, but with less overhead. NW.js must be installed separately. Be sure to install a recent version of NW.js for 64 bit CPU architecture, and select the SDK version of NW.js to get the JavaScript debugger.
7. A variety of unit generators, see the Csound Reference Manual for a listing. Not all are available on Windows. The vst4cs opcodes, which enable Csound to host VST plugins, are included.
8. CsoundVST, which enables Csound to be used as a VST plugin in VST hosts.

All programs and libraries contained in the Windows installer are built with Microsoft Visual Studio 2017 for 64 bit CPU architecture, and thus C++ components and interfaces of Csound are binary compatible with all other software built with MSVS 2017 for 64 bit CPU architecture.

# SELECTED CSOUND RESOURCES

Csound home page (including Csound for other operating systems) at http://csound.github.io/index.html. Contains links to many more resources!

CsoundQt home page at http://csoundqt.github.io/.

Csound on GitHub at https://github.com/csound/csound.

Csound for Android app on the Google Play Store at https://play.google.com/store/apps/details?id=com.csounds.Csound6&hl=en.

Online version of the Csound Reference Manual at https://gogins.github.io/csound/html/indexframes.html.

Online version of the Csound API Documentation at http://csound.github.io/docs/api/index.html.

FLOSS Manual for Csound at http://floss.booktype.pro/csound/preface/.

blue, a composition environment for Csound with extensive capabilities, at http://blue.kunstmusik.com/.

Cabbage, a Csound editor that can create VST plugins with custom interfaces, at http://www.cabbageaudio.com/.

