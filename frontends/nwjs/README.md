# CSOUND.NODE

Version 6.05.2

Michael Gogins<br>
http://michaelgogins.tumblr.com<br>
michael dot gogins at google dot com

Csound is free software; you can redistribute them
and/or modify them under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

Csound is distributed in the hope that they will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this software; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA

## INTRODUCTION

[NW.js][nwjs], which used to be called Node-Webkit, is a system for building applications
for personal computers and workstations based on Web browser technology. Typically
the user interface is defined in HTML and the program logic is defined in JavaScript.
C++ addons, which expose JavaScript functions that call into C++ libraries, also can
be used. The application executes in an embedded Web browser based on Google Chrome.

`csound.node` is a C++ addon for NW.js that embeds [Csound][csound] into the JavaScript context
of Web pages running in NW.js. Such pages can call core methods of the Csound API
as member functions of a `csound` object that belongs to the window. The
`csound/examples/html` directory contains a number of examples that run in NW.js with
`csound.node` (the CSD examples will also run on Csound for Android or in CsoundQt).

Therefore, NW.js with `csound.node` can be used not only for composing and performing
Csound pieces, but also for developing standalone applications that incorporate Csound.
It can be used, for example, to develop sound art installations, visual music, or kiosk-type
applications.

The `csound/examples/html/csound_editor` directory contains a NW.js application that
can be used to edit and run Csound pieces. Such pieces can be either HTML files
or Csound Structured Data (CSD) files. The `NW_Csound_Demo.html` piece is an example of
an HTML file that embeds not only Csound, but also a Csound orchestra and score. The
`GameOfLife3D.csd` piece is an example of a CSD file that embeds a Web page in the
`<html>` element of the CSD file.

The motivation for `csound.node` should be obvious. It works on all personal computer
platforms, the build steps are simple, and the
end product makes all of the myriad capabilities of HTML5 available to Csound pieces,
including JavaScript, user-defined HTML user interfaces, 3-dimensional animated computer
graphics, and much more. For a full list of capabilities currently implemented in HTML5, see
[this HTML5 test page][html5test].

## BUILDING

Currently, on Windows, there is no need to build `csound.node`, it is distributed in the
Windows installer for Csound. But you still need to install it, as described below.

If you do want to build `csound.node` on Windows, then you need to install [Microsoft Visual Studio][msvs]
and [Python 2.7][python].

To build on other platforms, you need to install [GCC][gcc] and [Python 2.7][python].

Install [NW.js][nwjs]. On Windows, make sure that this is the version for 32 bit CPU
architecture.

Install [io.js][iojs]. On Windows, make sure that this is the version for 32 bit CPU
architecture.

From the io.js command prompt, execute `npm install nw-gyp` to install the build tool
for NW.js addons. See [nw-gyp][] for more information.

From the io.js command prompt, execute `nw-gyp configure --version <version number of NW.js>`
to configure the build tool for your version of NW.js. The version number is printed
on the default window of the NW.js `nw` program.

Set an environment variable named `CSOUND_HOME` that points to the root directory of
your Csound installation.

In the `csound/frontends/nwjs` directory, execute `nw-gyp build` to build `csound.node`.
If the build messages end with `ok`, then the build succeeded.

## INSTALLING

Copy `csound.node` from the `csound/frontends/nwjs/build/Release` directory to your Csound
installaton's bin directory, or better yet, create a symbolic link to the built `csound.node`
file in your Csound installation's bin directory.

On Windows, copy `libgcc_s_dw2-1.dll` and `libstdc++-6.dll` from your Csound bin directory
to your NW.js directory, or create symbolic links there.

Set an environment variable named `NODE_PATH` that points to your Csound installation's
bin directory. This will bring both `csound.node` and Csound itself into
the scope of NW.js's addon loader.

Test your installation by running the NW.js program, `nw`. Drag the `NW_Csound_Demo.html` file
and drop it on `nw`'s window. You should hear the Csound piece "Xanadu" by Joseph Kung,
which is embedded in the HTML file, and you should be able to click on `nw`'s menu button
to bring up the NW.js developer tools, where you should see the runtime messages
from Csound being printed in the JavaScript console.

## RUNNING

There are several ways of running Csound pieces in NW.js with `csound.node`.

1. Run the `nw` program, and drop an HTML file that embeds a Csound piece on the `nw`
window.

2. From the command line, execute `nw <directory>`, where the directory contains the
JSON-formatted manifest of a NW.js application based on an HTML file that embeds a
Csound piece. See the NW.js documention for more information on the manifest and
packaging NW.js applications.

3. Run the `csound_editor` application, either by executing `nw /csound/examples/html/csound_editor`,
or by dropping the `/csound/examples/html/csound_editor/main.html` file on `nw`'s window. Then,
click on the Open button to load either a HTML file embedding a Csound piece, or a CSD file
embedding a Web page in the CSD file's `<html>` element. Click on the Run button to play the
piece, and click on the Stop button to stop Csound. Csound will also stop if you close the
window for the piece.

[csound]: http://csound.github.io/
[nwjs]: http://nwjs.io/
[iojs]: https://iojs.org/en/index.html/
[msvs]: https://www.visualstudio.com/
[html5test]: https://html5test.com/
[gcc]: https://gcc.gnu.org/
[python]: http://www.python.org/
[nw-gyp]: https://github.com/nwjs/nw-gyp/
