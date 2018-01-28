Android: build scripts and sources for the Android platform
=======================================

* build-all.sh: builds Csound for Android as well as plugins
* downloadDependencies.sh: downloads dependencies for plugins
* release.sh: creates a release package

Directories
----------

* CSDPlayer: simple CSD player example
* CsoundAndroid: Android Csound JNI and java interface
* CsoundAndroidExamples: suite of examples and tests
* pluginLibs: Csound plugin opcode ports


Android Toolchain
-------------------

The following are the required toolchain components to build Csound.

- Android Studio: https://developer.android.com/studio/index.html.

- Android Native Development Kit (NDK): http://developer.android.com/tools/sdk/ndk/index.html.

Build Instructions
----------------

Before building, make sure ANDROID_NDK_ROOT contains the path to the
installed NDK. The variable NDK_MODULE_PATH should also be set to
the location of the NDK modules (libraries) that will be used in the
build, such as libsndfile (the only dependency for Csound).

1. If you want to build the Csound library only (requires the libsndfile-android
NDK module, which has to be built separately):

```
$ cd ../CsoundAndroid
$ sh build.sh
```

2. If you want to build the ported plugin opcode libraries as
well:

```
$ sh downloadDependencies.sh
$ sh build-all.sh
```

Once the Csound JNI library is used, the CsoundAndroid project is ready to
be used in an Android Studio application. Just add it as a dependency.
