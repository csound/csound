Building the vst4cs Opcodes and CsoundVST on Linux
==================================================

The vst4cs opcodes which enable Csound to host VST plugins work just fine on Linux. So does CsoundVST which enables Csound itself to run as a VST plugin. For a possibly incomplete but very useful list of VST plugins (and VST hosts) available on Linux, see http://linux-sound.org/linux-vst-plugins.html.

Please be aware that the Csound license permits building and linking code from Steinberg's VST SDK and Hermann Seib's VST Host, see the license text in the vst4cs and CsoundVST source code.

To build and install the VST features on Linux:

1. Ensure that your environment and software are consistent with respect to CPU architecture. Csound on Linux by default is built for 64 bit CPU architecture, e.g. `amd64`, and for double-precision audio samples. Your target VST plugins and hosts should also be built for 64 bit CPU architecture.

2. Download Steinberg's VST SDK from https://www.steinberg.net/en/company/developers.html, e.g. https://download.steinberg.net/sdk_downloads/vstsdk368_08_11_2017_build_121.zip. Unpack the zip file in your home directory.

3. Patch ~/VST_SDK/VST2_SK/pluginterfaces/vst2.x/aeffect.h as follows. At about line 67, change

```
#elif defined(__GNUC__)
```
to 
```
#elif defined(__MINGW32__)
```

4. Do a fresh pull from the Csound GitHub repository.

5. Now you can build Csound with the VST features included. In your Csound build directory:

   1. Configure CMake to build Csound with the VST features. Delete your `CMakeCache.txt` file. On the cmake command line, add: `-DVSTSDK2X_INCLUDE_DIR:PATH=~/VST_SDK/VST2_SDK -DBUILD_CSOUND_VST=1 -DBUILD_VST4CS_OPCODES=1`. You can also configure this using cmake-gui.  Run cmake to regenerate your makefiles.

   2. Run `make`.

   3. Run `sudo make install.`




