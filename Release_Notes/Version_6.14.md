<!---

To maintain this document use the following markdown:

# First level heading
## Second level heading
### Third level heading

- First level bullet point
 - Second level bullet point
  - Third level bullet point

`inline code`

``` pre-formatted text etc.  ```

[hyperlink](url for the hyperlink)

Any valid HTML can also be used.

 --->
======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ======== DRAFT ========

# CSOUND VERSION 6.14 RELEASE NOTES

-- The Developers

## USER-LEVEL CHANGES

### New opcodes

- randc is like randi but uses a cubic interpolation.

- mp3out is an experimental mplementation of writing an mp3 file.  It
  may be replaced  by the current work in libsndfile to deal with MPEG
  files.

- metro2 is likre metro but with added controllable swing.

- ftexists reports whether a numbered ftable exists.

### New Gen and Macros

### Orchestra

- The conditional expresson syntax a?b:c incorrectly always
  calculated b and c before selecting which to return.  This could
  give incorrect devision by zero errors or cause unexpected multiple
  evaluations of opcodes.  It now impements the common C-like semantics.

### Score

- 
  
### Options

- New option simple-sorted-score creates file score.srt in a more
  user-friendly format

- Revise treatment of CsOptions wrt double quotes and spaces which need escaping.

- Setting the 1024 bit in -m suppresses printing of messages about
using deprecated opcodes.  This option is itself deprecated.

### Modified Opcodes and Gens

- squinewave now handles optional a or k rate argument.

- pindex opcode handls string fields as well as numeric ones.

- sflooper reworked to avoid a crash and provide warnings.

- event_i and schedule can rake fractional p1.

- in sound font opcodes better checking.   Also no longer will load
  multiple copies of a sound font, but reuses existing load.

- fluidControl has a new optional argument to control printing
  messages.

- bpf has an audio version now.

- stsend/stecv can work with unmatched k-rates.

- pvstrace has new optional arguments.

- lpfreson checks number of poles.

- syncloop had a small typing error that caused crashes.

- bpfcs has new array versions.


### Utilities

- lpanal now checks that suficient poles are requested.

### Frontends

- Belacsound:

- CsoundQt:

### General Usage

- // comments at rthe start of a line now accepted in CsOptions
  section of a csd file. 

## Bugs Fixed

- shiftin fixed.

- exitnow delivers the return code as documented.

- fixed bug in beosc, where gaussian noise generator was not being
  initialised.

- OSCraw fixed.

- ftkloadk could select incorrect interal code causing a crash.

- GEN01 when used to read a single channel of a mlti-channel file got
  the length incorrect.

- ftgenonce had a fencepost problem so it could overwrite a table in
  use.

- a race condition in Jacko opcodes improved (issue #1182).

- syncloop had a small typing error that caused crashes.

- lowresx was incomplete and did not work as intended; rewritte (issue #1199)

# SYSTEM LEVEL CHANGES

-

### System Changes

- New plugin class for opcodes with no outputs written.
erform time errors and int errors are also reorte in te retur code of
the command line system.  The new API function GetErrorCnt is
available to do somting similar in other varants.
- 

### Translations

### API

- Function GetErrorCnt gives the number of perf-time errors, and adds
  in the init-time errors at the end of a rendering.

- Function FTnp2Find no longer print smessages if the table is not
found, but just returns NULL.  Previous behavour is available  as
FTnp2Finde.

- csoundGetInstrument() added

### Platform Specific

- WebAudio: 

- iOS

- Android

- Windows
 - stsend reworked for winsock library

- MacOS

- GNU/Linux

- Haiku port

- Bela

==END==

-----------------------------------------------------------------------
------------------------------------------------------------------------

commit 559f215f4273a8e585583be57db776c1dfa1ae29 (HEAD -> develop, origin/develop
, origin/HEAD)
Author: François PINOT <fggpinot@gmail.com>
Date:   Mon Dec 9 18:12:51 2019 +0100

commit 526b0ca658da993e55c467629d01d3d8092ffb9a
Author: Eduardo Moguillansky <eduardo.moguillansky@gmail.com>
Date:   Wed Dec 4 16:42:00 2019 +0100

    outvalue: koutval is called only once during the init phase. Also the order of
    the declaration of the opcode variants in Engine/entry1.c was changed to allow
    outvalue.Si to take precedence over outvalue.k for when used "outvalue Schan, ivalue"
    (outvalue.k is still used for outvalue Schan, kvar)

commit ad5e171bb1e9c0a5e4d5419531344db0b32352dd
Author: Steven Yi <stevenyi@gmail.com>
Date:   Tue Dec 3 11:02:15 2019 +0100

    expose csoundCompile with extra commandline arg to allow overriding -o if it is in CsOptions

commit 5cdd1a274b4ddb389ee3277905d61287c82ecbf1
Author: Shantanu Jamble <sjamble24@gmail.com>
Date:   Mon Nov 18 21:43:36 2019 -0500

    Csound Azure & github actions pipelines (#1227)
    
    * Added dockerfiles for the ubuntu and emscripten
    
    * Restructured directories
    
    * Github actions workflow test
    
    * Set up CI with Azure Pipelines (#1)
    
    * Set up CI with Azure Pipelines
    
    * Update emscripten.yml (#3)
    
     Update csound_builds.yml
    
    * Update ReadMe.md
    
    * Update README.md
    
    * Update csound_builds.yml (#4)

commit 5f351d4594ea6294f846685e8637a1ef46ad3a1b
Author: Stephen Kyne <xskyne@gmail.com>
Date:   Mon Nov 18 20:07:13 2019 +0100

    MSVC build changes (#1208)
    
    * Added libsndfile cmake changes
    
    * Added FindLIBLO.cmake
    
    * Removing old build script build2015.bat
    
    * HDF5 cmake changes for VCPKG
    
    * Added multiple VCPKG packages to downloadDependencies.ps1 file
    
    * MSVC: Minor comments and changes to downloadDepdencies.ps1
    
    * MSVC: Remove HDF5 manual build from downloadDependencies.ps1
    
    * Fixed HDF5 cmake file for windows
    
    * Added dirent as vcpkg, removed old checked in file
    
    * Added chocolatey as the package provider for larger dependencies like swig and flex/bison
    
    * Removed WIP section in README.md
    
    * Cleaned up build files for MSVC. Removed old manual builds and minor formatting fixes.
    
    * Removed unneeded flags
    
    * Fixed FindLiblo.cmake and now using this to find liblo instead of manually sourcing header and library
    
    * Minor cleanup for FindLiblo
    
    * Added FindPortMidi.cmake and now using find_package for this dependency in cmake files
    
    * Fixed minor text error in cmakefile for liblo libraries
    
    * MSVC: Removing any PATH modification from build files
    
    * Appveyor: Reverse deletion of some variable helpers
    
    * MSVC: Completion of portmidi vcpkg integration. Now using find_package with appropriate variables
    
    * MSVC: Small changes to make linear opcodes compile
    
    * MSVC: Added GMM dependency check in cmake fiel
    
    * Appveyor: Cache entire VCPKG dir like before. Change seems to always invalidate cache
    
    * MSVC: Removed unneeded include in cmake file
    
    * Revert "MSVC: Removing any PATH modification from build files"
    
    This reverts commit d01d91242844c7ed9d689281d56ee0986c2f8a4f.
    
    * Added semi-colon to fix path error
    
    * One more attempt at fixing path
    
    * Updated project and appveyor to use VS 2019
    
    * Remove usage of deprecated swig cmake command and use newer version
    
    * Increase chocolatey timeout for slow packages
    
    * Updated inno setup location, fixed typo
    
    * Undo "typo"
    
    * Updated VC redist appveyor details
    
    * Cmake: Use list(append) instead of set
    
    * Cmake tweaks to get linux to use the find_* modules
    
    * Fix for GMM finding in cmake code
    
    * Fix for eigen package finding in cmake
    
    * Minor cmake syntax tweaks
    
    * Update last swig_add_module usage
    
    * Revert "Remove usage of deprecated swig cmake command and use newer version"
    
    This reverts commit 5201a4e5473537fedb4fb94c788305f9dfc0315e.
    
    * Remove commented script code
    
    * Revert "Update last swig_add_module usage"
    
    This reverts commit 7feb8c116ed5ce27dccf8edcfd8918bb48ecb477.
    
    * Eigen fix for non windows platforms
    
    * Add cmake logging to show module path
    
    * Added eigen3 find cmake file for non-msvc platforms
    
    * Reverted to use generic find_package mechanism for eigen for all platforms
    
    * Fix swig cmake warnings
    
    * Remove accidental test comment
    
    * Add debug cmake logging for find packages
    
    * Revert "Add debug cmake logging for find packages"
    
    This reverts commit 83146848cb08690f1fedf348fb365aed5f62a8c2.
    
    * Debug cmake logging for find_package for travisci
    
    * Trace logging for linux/osx cmake
    
    * Minor syntax change for module folder
    
    * Revert "Trace logging for linux/osx cmake"
    
    This reverts commit 5e8412df1b6e27699c325a15e29e49da437ef57e.
    
    * Clean up HDF5 cmake file. Shouldn't be a required dependency.
    
    * Add debug logging to cmake
    
    * Cmake changes to deal with unfound dependencies
    
    * Added some debug cmake logging for liblo for osx
    
    * Revert "Added some debug cmake logging for liblo for osx"
    
    This reverts commit 23c27d44b687cdd4d36c74c04ed238bfeac6fbf0.
    
    * Fix for FindLiblo.cmake. Incorrectly setting found variable when not the case.
    
    * Remove debug cmake logging
    
    * Update build.bat to use newer VS
    
    * Remove toolset parameter usage from scripts
    
    * Added asiosdk download and extraction to VCPKG directory for portaudio
    
    * Updated documentation for build changes
    
    * Remove unneeded parameters
    
    * Added FindPortaudio.cmake and switched to using find_package in cmakelists
    
    * Renamed cmake module files to match lower case usage
    
    * Revert "Renamed cmake module files to match lower case usage"
    
    This reverts commit 4180ab7d90a08b2f1ba121a0365e10285d61dbe1.
    
    * Cmake modules, fixed casing of export targets and find modules
    
    * Fixes for portaudio. Using package variables to determine whether to build or not
    
    * Remove unused toolset parameter in powershell "generateProject"
    
    * Remove redundant find_package for swig
    
    * Remove unneeded find_package calls. changed casing on find_package calls to find correct modules
    
    * Fix for portaudio finding in cmake
    
    * Further fixes for portaudio cmake
    
    * Undo case change for find_package faust target
    
    * Prevent library searching on non-relevant platforms
    
    * Cleaned up windows libraries lists
    
    * MSVC build now builds with a static CRT runtime instead of dynamic (MD)
    
    * Added stk VCPKG
    
    * Remove C++11 flag for MSVC, it doesn't support it (only c++14/17/latest)
    
    * Fix for STK headers
    
    * Another attempt to fix STK header inclusion
    
    * Clear VCPKG cache on appveyor
    
    * Revert "Clear VCPKG cache on appveyor"
    
    This reverts commit deb0956c985d871344bd6d8ee4725f47a87883f7.
    
    * Fixed up CMake formatting
    
    * Removed generateProject powershell. Condensing it all into one script
    
    * Setting VCPKG repo to a specific commit to have more consistent builds
    
    * Added project generation to downloadDependencies script due to shared variables usage. Added params for static/dynamic building and CRT linkage. Single file guarantees proper triplet usage also
    
    * Fixed up portaudio asiosdk code in PS script. Also reduced downloading if file is present
    
    * Removed unneeded code and touched up formatting
    
    * Added new triplet in the case of a dynamic library build with a static CRT linkage
    
    * Avoiding finding dependencies for non-valid platforms
    
    * Added cmake code to change CRT linkage for the project to static or dynamic based on command line parameter.
    
    * Cleaned up windows libs cmake code. No need to manually add CRT libs
    
    * Added missing HDF5 libs for msvc
    
    * Remove call to deleted script in Appveyor config
    
    * Minor portaudio cmake change
    
    * Change build type to Release instead of ReleaseDebugInfo as this isn't supported by VCPKG
    
    * Installer changes for build type change to Release
    
    * Removing remenants of manual builds. Everything is now in vcpkg or chocolatey (bar GMM)
    
    * Portaudio workaround for vcpkg
    
    * Added fluidsynth find cmake module and updated cmakelists
    
    * Removed unneeded path location in Appveyor config
    
    * Updated logging, documentation
    
    * Fixed up install script to match other changes
    
    * Minor cleanups for appveyor script
    
    * Uncomment CRT files copying for installer script
    
    * Fix for logging in downloadDependencies script
    
    * Including vcpkg dlls into installer script
    
    * Added fix to find dirent.h which is now got via VCPKG. This was breaking the dynamic loading of plugins silently. Added message if not found to MSVC and other build types
    
    * Minor fix to remove adding dirent path to cmake variable if not msvc

commit 88c1f746891cbb121691b8ebbccda48b4862daac
Author: roryywalsh <rorywalsh@ear.ie>
Date:   Wed Nov 13 10:33:07 2019 +0000

    adding k-time checking for channel strings in chnget and chnset opcodes

commit a4706809398daf2eddb0b778bfe4e2c57d3dce64
Merge: c04f371afd dbd36d56e2
Author: Pete Goodeve <pete.goodeve@computer.org>
Date:   Tue Oct 29 17:16:21 2019 -0700

    resolve sfont.c confusion

commit 683eeb5a6605e6b179894a2438e496d6b4a9f3dc
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Tue Sep 24 20:38:20 2019 +0100

    added some more functionality to CPOF

commit 797f3098efdc06e587fe8d45a4b9b6b4fc4daec0
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 19 13:53:55 2019 -0400

    added getPlayState(), addPlayStateListener(), and other methods for querying and listening to the playing state of Csound

commit cbc3af7c5b80b5073f187604bf3b60a00539c6d6
Author: Steven Yi <stevenyi@gmail.com>
Date:   Thu Sep 12 10:56:37 2019 -0400

    store temp to prevent dangling pointer [issue #1181]

commit d1260b62724bdce9281a9a6e046b3443b0f4544e
Author: Steven Yi <stevenyi@gmail.com>
Date:   Wed Sep 4 15:58:56 2019 -0400

    fix for message printing when using ScriptProcessorNode [fixes #1174]


commit 8eca5dda97c20118b85db97943f6a53e62e9aa74
Author: veplaini <victor.lazzarini@nuim.ie>
Date:   Sun Jul 28 21:41:19 2019 +0100

    fixed opcode registration in CPOF

