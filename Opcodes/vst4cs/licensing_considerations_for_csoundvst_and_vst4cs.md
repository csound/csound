#LICENSING AND HOSTING CSOUNDVST AND VST4CS#
Michael Gogins
22 September 2017

This is complicated and tedious, so please read carefully and pay attention. I have read all the licenses and agreements from Steinberg, AppVeyor, and GitHub as well as the LGPL v2.1, several times. Please note, I am not a lawyer; so if you think you need legal advice, get it from a lawyer. I welcome comments and corrections. 

##SUMMARY##

If you're going to go to sleep, here's the tl;dr:

-- Hosting source code for CsoundVST and the vst4cs opcodes on GitHub does not violate Steinberg's VST SDK 2.4 license, which is the only one we need to comply with, because we don't host the VST SDK code.

-- Hosting source code for CsoundVST and the vst4cs opcodes on GitHub does not violate Csound's GPL v2.1 license.

-- Hosting binaries for Csound, CsoundVST, and the vst4cs opcodes on AppVeyor does not violate Steinberg's VST SDK 2.4 license, which is the only one we need to comply with, again because we don't redistribute the VST SDK code.

- The question whether hosting binaries for CsoundVST and the vst4cs opcodes violates Csound's LGPL v2.1 license is more complex. However, it is permitted by clause 6 of the LGPL v2.1 and as indicated by https://www.gnu.org/licenses/gpl-faq.en.html#GPLIncompatibleLibs. 

###Required Modification###

We should however modify the wording of our LGPL v2.1 license for CsoundVST and vst4cs to include an exception for using the VST2 SDK. It would also be possible to use the VST3 SDK's GPL v3 license, but that would require porting both CsoundVST and the vst4cs opcodes to use VST SDK3 instead of the current VST2 SDK. In that case, it is clear that the Csound library could load the vst4cs opcodes, but it is not as clear that non-GPL hosts could load CsoundVST, although this situation is common in the wild.

Assuming that we continue to use the LGPL v2.1 license, the licenses for all Csound source code in the `frontends/CsoundVST` and `Opcodes/vst4cs` directories should be changed to read (text is worded for vst4cs):

```
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking vst4cs statically or dynamically with other modules is making a 
//  combined work based on vst4cs. Thus, the terms and conditions of the GNU 
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of vst4cs give 
//  you permission to combine vst4cs with free software programs or libraries
//  that are released under the GNU LGPL and with code included in the 
//  standard release of the VST SDK version 2 under the terms of the license 
//  stated in the VST SDK version 2 files. You may copy and distribute such a 
//  system following the terms of the GNU LGPL for vst4cs and the licenses of 
//  the other code concerned. The source code for the VST SDK version 2 is 
//  available in the VST SDK hosted at https://github.com/steinbergmedia/vst3sdk.
//  
//  Note that people who make modified versions of vst4cs are not obligated to 
//  grant this special exception for their modified versions; it is their 
//  choice whether to do so. The GNU Lesser General Public License gives 
//  permission to release a modified version without this exception; this 
//  exception also makes it possible to release a modified version which 
//  carries forward this exception.

```

##DETAILED EXPLANATION##

OK, here are the grisly details.

The licenses and agreements that we need to comply with are:

1. GitHub's terms of use.

2. AppVeyor's terms of use.

3. The license used by the Steinberg VST2 SDK (not the VST3 SDK, I will explain this).

4. Csound's LGPL v2.1 license, which specifically includes the clause "or (at your option) any later version."

I will now explain these in more detail.

Regarding AppVeyor, its terms of service require only that we truthfully convey to AppVeyor the copyrights and licenses to all code built on AppVeyor. This automatically happens when AppVeyor clones Csound's Git repository. So, we do this.

Regarding GitHub, its terms of service similarly make no requirements as to licensing of hosted code, but recommends that hosted projects use an open source license. We do that.

Csound does not use the GPLv3 license, but rather the LGPL v2.1 license. GPL is "free software," LGPL is "open source." The GPL license is compatible with the LGPL v2.1 license if and and only if the LGPL license includes the wording "or (at your option) any later version." Our license does include this wording. So Csound itself is compatible with the GPL v3 -- if we convey the Csound code under GPL v3. The LGPL allows LGPL or proprietary software to use the Csound library. The situation might be different if CsoundVST and the vst4cs opcodes were GPL. The language for dynamic linking in the GPL is imprecise and contested. Both CsoundVST and vst4cs are not shared libraries that are linked at compile time, they are dynamically loaded and linked at run time. Opinions differ on whether this is permitted. If not, then CsoundVST under GPL 3 could not be loaded by GPL-incompatible hosts.

NOTE: We have to worry not only about Csound -- but also about a lot of third party dependencies such as libsndfile!
    
Steinberg's license for the VST SDK is different for different files in the SDK. The VST SDK distribution comes in two directories: VST2_SDK and VST3_SDK. EACH OF THESE HAS ITS OWN LICENSE! Specifically, the VST2_SDK files are licensed thus:

//-----------------------------------------------------------------------------
// LICENSE
// (c) 2017, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// This Software Development Kit may not be distributed in parts or its entirety  
// without prior written agreement by Steinberg Media Technologies GmbH. 
// This SDK must not be used to re-engineer or manipulate any technology used  
// in any Steinberg or Third-party application or software module, 
// unless permitted by law.
// Neither the name of the Steinberg Media Technologies nor the names of its
// contributors may be used to endorse or promote products derived from this 
// software without specific prior written permission.
// 
// THIS SDK IS PROVIDED BY STEINBERG MEDIA TECHNOLOGIES GMBH "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL STEINBERG MEDIA TECHNOLOGIES GMBH BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------------

The VST3_SDK files are licensed thus:

//-----------------------------------------------------------------------------
// LICENSE
// (c) 2017, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
This license applies only to files referencing this license,
for other files of the Software Development Kit the respective embedded license text
is applicable. The license can be found at: www.steinberg.net/sdklicenses_vst3

This Software Development Kit is licensed under the terms of the Steinberg VST3 License,
or alternatively under the terms of the General Public License (GPL) Version 3.
You may use the Software Development Kit according to either of these licenses as it is
most appropriate for your project on a case-by-case basis (commercial or not).

a) Proprietary Steinberg VST3 License
The Software Development Kit may not be distributed in parts or its entirety
without prior written agreement by Steinberg Media Technologies GmbH.
The SDK must not be used to re-engineer or manipulate any technology used
in any Steinberg or Third-party application or software module,
unless permitted by law.
Neither the name of the Steinberg Media Technologies GmbH nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.
Before publishing a software under the proprietary license, you need to obtain a copy
of the License Agreement signed by Steinberg Media Technologies GmbH.
The Steinberg VST SDK License Agreement can be found at:
www.steinberg.net/en/company/developers.html

THE SDK IS PROVIDED BY STEINBERG MEDIA TECHNOLOGIES GMBH "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL STEINBERG MEDIA TECHNOLOGIES GMBH BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

b) General Public License (GPL) Version 3
Details of these licenses can be found at: www.gnu.org/licenses/gpl-3.0.html
//----------------------------------------------------------------------------------

Csound's VST features do not use any files in VST3_SDK, but only files in VST2_SDK. I did not sign the agreement for VST SDK 3, because I do not use it. I do adhere to the terms of the VST SDK 2.4, which I registered for and downloaded from Steinberg a long time ago.

Please note, we COULD license CsoundVST and the vst4cs opcodes as GPL v3 because of the "any later version" clause of our LPGL v2.1 license, thus complying with the VST3 SDK GPL v3 license, but that would require porting both CsoundVST and the vst4cs opcodes to use the VST3 SDK.

So the main question is whether Csound's LGPL v2.1 license permits us to host and distribute the existing CsoundVST and vst4cs binaries. The issue is complex, and the law does not appear to be completely settled.

As background, be aware that CsoundVST is a "derivative work" of both the csnd6 library and the VST2 SDK, because it incorporates source code from both. CsoundVST is NOT a derivative work of the csound64 library, because it does not incorporate source code from it, but only uses the csound64 library by dynamic linking. Opinions differ as to dynamic linking, but a strong argument can be made that a program that uses a LGPL v2.1 library by dynamic linking is not a derivative work. Case law leans in that direction, but has so far not squarely focused on this issue.

The vst4cs opcodes are NOT a derivative work of any part of Csound, but ARE a derivative work of the VST2 SDK. The vst4cs source code, however, like Csound, is LGPL v2.1.

The question then is whether the LGPL v2.1 permits us to host the binaries for CsoundVST and the vst4cs opcodes on AppVeyor and/or GitHub. At first sight, the fact that CsoundVST is a derivative work of the csnd6 library but incorporates proprietary code, and the fact that the vst4cs opcodes are themselves LGPL v2.1 but incorporate proprietary code, would seem to mean "no." 

However, NOT SO FAST, the LGPL v2.1 license includes a clause to deal with this kind of situation. My comments are enclosed in ***.

6. As an exception to the Sections above, you may also combine or link a "work that uses the Library" with the Library to produce a work containing portions of the Library, and distribute that work under terms of your choice, provided that the terms permit modification of the work for the customer's own use and reverse engineering for debugging such modifications. *** THIS IS THE KEY. We do this. ***

You must give prominent notice with each copy of the work that the Library is used in it and that the Library and its use are covered by this License. You must supply a copy of this License. If the work during execution displays copyright notices, you must include the copyright notice for the Library among them, as well as a reference directing the user to the copy of this License. Also, you must do one of these things:

a) Accompany the work with the complete corresponding machine-readable source code for the Library including whatever changes were used in the work (which must be distributed under Sections 1 and 2 above); and, if the work is an executable linked with the Library, with the complete machine-readable "work that uses the Library", as object code and/or source code, so that the user can modify the Library and then relink to produce a modified executable containing the modified Library. (It is understood that the user who changes the contents of definitions files in the Library will not necessarily be able to recompile the application to use the modified definitions.)
b) Use a suitable shared library mechanism for linking with the Library. A suitable mechanism is one that (1) uses at run time a copy of the library already present on the user's computer system, rather than copying library functions into the executable, and (2) will operate properly with a modified version of the library, if the user installs one, as long as the modified version is interface-compatible with the version that the work was made with. *** This is true of Csound, CsoundVST, and vst4cs. ***
c) Accompany the work with a written offer, valid for at least three years, to give the same user the materials specified in Subsection 6a, above, for a charge no more than the cost of performing this distribution.
d) If distribution of the work is made by offering access to copy from a designated place, offer equivalent access to copy the above specified materials from the same place. *** We do this for everything but the VST2 SDK; and we also instruct users how to copy the VST2 SDK from Steinberg. There are a number of other VST plugins on GitHub, with both open source licenses and free software licenses, that do just this, including the widely used JUCE audio application framework, which is GPL v3. ***
e) Verify that the user has already received a copy of these materials or that you have already sent this user a copy.
For an executable, the required form of the "work that uses the Library" must include any data and utility programs needed for reproducing the executable from it. However, as a special exception, the materials to be distributed need not include anything that is normally distributed (in either source or binary form) with the major components (compiler, kernel, and so on) of the operating system on which the executable runs, unless that component itself accompanies the executable.

It may happen that this requirement contradicts the license restrictions of other proprietary libraries that do not normally accompany the operating system. Such a contradiction means you cannot use both them and the Library together in an executable that you distribute. *** We are not in this kind of contradiction, because we DO comply with the VST2 SDK license. ***

The AppVeyor.yml file is set up to download the VST SDK 3. I believe that we may download the VST SDK 3, not use any of the SDK 3 specific files, use only the SDK 2.4 files, and thus be bound only by the terms of the SDK 2.4 license.

So I think we are OK to build VST features for Csound on AppVeyor using only the VST SDK 2.4 files, and distribute the resulting binaries, without violating Steinberg's license. I don't think Steinberg is being very clear about this situation, because they want everyone to ask permission and thus explicitly give up liability claims against Steinberg. I would be fine with doing that, I just don't think it's necessary.

As far as Csound's LGPL v2.1 license is concerned, I believe we are OK under clause 6 of the license, but we could also license CsoundVST and the vst4cs opcodes under GPL v3 and then port them to use VST SDK3.

##OTHER PROJECTS##

To gain more understanding of the licensing issues, I have looked over a variety of other free software and open source projects for VST plugins and hosts.

Hermann Seib's VSTHOST http://www.hermannseib.com/english/vsthost.htm is LGPL v2.1 with the "later version" option and thus is completely compatible with Csound. That's good, because vst4cs is based on vsthost. It is ditributed both as source (without the VST2 SDK) and as a compiled program from Seib's Web site. However, according to the GPL, we need to get permission from Seib to modify our license as I have proposed. Update: Seib just gave his permission; I have appended his email below.

The Pure Data extension `vst~`, which serves the same purpose as vst4cs, hosted at https://sourceforge.net/p/pure-data/svn/HEAD/tree/trunk/, simply incorporates minimal VST2 SDK files attributed to Steinberg, but without license or explanation. Pure Data itself uses the BSD license which is less restrictive than LGPL v2.1.

Psycle at https://sourceforge.net/projects/psycle/ is GPL v2 but uses a later version of Seib's VSTHost that is GPL v2.1.

Some of the `vst~` external is based on code from http://iem.kug.ac.at/~jams/, but currently this page is inaccessible.

Qtractor at https://sourceforge.net/projects/qtractor/ contains the VeSTige aeffect.h header which appears to reverse engineer the VST2 SDK. However, the Qtractor README says it uses Steinberg's VST2 SDK.

The JUCE framework, which contains features similar to both CsoundVST and vst4cs, appears to use the Steinberg VST3 SDK which appears to be consistent with the JUCE license, especially if GPL v3.

The LMMS sequencer is GPL v2. It appears to contain the VeSTige emulation of the VST2 SDK which is GPL v2.

Paul Nasca's http://zynaddsubfx.sourceforge.net/ now provides VST support via something called DPF, which I think stands for Distrho Plugin Framework or something like that. The DPF directory in the ZynAddSubFX repository is empty. 

The DPF uses VeSTige from Ardour. The DPF uses the ISC license, which is very permissive open source. But it can also use the VST2 SDK.

Ardour at https://github.com/Ardour/ardour uses the GPL v2. Ardour also uses VeSTige from LMMS which is GPL v2.

WDL-OL at https://github.com/olilarkin/wdl-ol is an audio plugin development framework that uses, among other libraries, the VST SDK. WDL-OS uses the Cockos WDL License which is a very permissive open source license. The programmer must download the VST SDK from Steinberg and also copy the most basic VST2 SDK files. NOTE: It might make sense to use this for Csound and gain interoperability with a number of plugin formats.

Dexed at https://github.com/asb2m10/dexed uses the JUCE plugin wrapper and also is GPL v3.

vst2413 at https://github.com/keijiro/vst2413 has no license information. It appears to use the VST2 SDK. By the way, it sounds great. Releases include binaries.

rust-vst2 at https://github.com/overdrivenpotato/rust-vst2 is a "VST 2.4 API implementation in rust. Create plugins or hosts." Uses the MIT license. The implication is that overdrivepotato has either ported or reverse engineered the VST2 SDK.

amsynth at https://github.com/amsynth/amsynth. It uses GPL v2 and also uses VeSTige.

VOSIMSynth at https://github.com/austensatterlee/VOSIMSynth uses GPL v3. The programmer must download the VST SDK from Steinberg and copy some VST2 SDK files. 

###TRENDS###

VeSTige is getting a lot of use, but is GPL v2.

JUCE is getting even more use, but makes projects GPL v3.

There are a number of projects that require the programmer to download and install VST2 SDK sources before compiling, as I propose.

##APPENDIX##

Here is the full text of my email to, and response from, Hermann Seib:

<him@hermannseib.com>
9:58 AM (24 minutes ago)

 to me 
Legal considerations always give me a headache.

Originally, my open-source VSTHost code contained no license at all - a "do what the fuck you want with it" license, so to speak. Which was taken advantage of, so I added a license; the sole intention of that was to add an explicit "... but don't pretend it's your code and make others pay for it!".

So ... your license below looks good to me. I had no problem when Psycle included parts of VSTHost, I don't have a problem with CSound including parts of it.

As long as it's understood that this license doesn't cover my own VSTHost code, i.e., it only covers your version and everything that's derived from it, you have my expressed permission.

I hope you don't need that in hand-written form, signed with blood :-)

Bye,

  Hermann


Am 23.09.2017 um 03:59 schrieb Michael Gogins:
I am a developer and maintainer of Csound, a widely used programmable
software synthesizer, see http://csound.github.io/ and
https://github.com/csound/csound. I am writing this to ask your
permission to add an exception to our licensing tems for code that we
have borrowed from your VSTHost code.

Currently, Csound contains vst4cs, a set of opcodes for csound, that
enable Csound to host VST instruments and effects. The sources for
vst4cs, in https://github.com/csound/csound/tree/develop/Opcodes/vst4cs,
are based in part on your VSTHost sources. The Csound license is LGPL
v2.1 with the "either version 2.1 of the License, or (at your option)
any later version" clause. I am currently the maintainer of vst4cs.

By the way, thank you for creating this excellent project, which we
use not only as the basis for vst4cs, but also for testing the VST
plugin version of Csound, CsoundVST.

Of course, we do not redistribute the VST2 SDK. To build vst4cs
requires the developer also to download the VST2 SDK from Steinberg.
The licensing situation then becomes a little tricky. According to the
GPL, we are permitted to modify the LGPL v2.1 text to permit other
users of vst2cs to also use the VST2 SDK and still keep the LGPL v2.1
license. However, we need the permission of the "original author" in
order to make this change. Our license notice would then read:

//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his Vst Host program
//  and from the vst~ object by Thomas Grill,
//  which in turn borrows from the Psycle tracker.
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking vst4cs statically or dynamically with other modules is making a
//  combined work based on vst4cs. Thus, the terms and conditions of the GNU
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of vst4cs, 
//  including the Csound developers and Hermann Seib, the original author of 
//  VSTHost, give you permission to combine vst4cs with free software programs 
//  or libraries that are released under the GNU LGPL and with code included 
//  in the standard release of the VST SDK version 2 under the terms of the 
//  license stated in the VST SDK version 2 files. You may copy and distribute 
//  such a system following the terms of the GNU LGPL for vst4cs and the 
//  licenses of the other code concerned. The source code for the VST SDK 
//  version 2 is available in the VST SDK hosted at
//  https://github.com/steinbergmedia/vst3sdk.
//
//  Note that people who make modified versions of vst4cs are not obligated to
//  grant this special exception for their modified versions; it is their
//  choice whether to do so. The GNU Lesser General Public License gives
//  permission to release a modified version without this exception; this
//  exception also makes it possible to release a modified version which
//  carries forward this exception.

May we have your permission to make this our license for our code
which is based on your code?

Thank you for your consideration,
Michael Gogins


-----------------------------------------------------
Michael Gogins
Irreducible Productions
http://michaelgogins.tumblr.com
Michael dot Gogins at gmail dot com

