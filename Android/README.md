Csound for Android 7.0
=======================================

Csound for Android consists of the Csound library, dedicated
OpenSL audio backends, a java interface, some selected
plugin libraries, and some test/example applications.

The following are the required toolchain components to build Csound:

- Android Studio: https://developer.android.com/studio/index.html.

- Android Native Development Kit (NDK): http://developer.android.com/tools/sdk/ndk/index.html.

Build Instructions
----------------

Before building, make sure ANDROID_NDK_ROOT contains the path to the
installed NDK. The variable NDK_MODULE_PATH should also be set to
the location of the NDK modules (libraries) that will be used in the
build.

```
$ sh downloadDependencies.sh
$ cd ../CsoundAndroid
$ sh build.sh
```


Once the Csound JNI library is used, the CsoundAndroid project is ready to
be used in an Android Studio application. Just add it as a dependency.


Directories
----------

* CsoundAndroid: Android Csound JNI and java interface
* CsoundForAndroid: Android Studio projects
* docs: documentation


Release Script
----

The `release.sh` can be used to create a release package containing
the CsoundForAndroid Android Studio project. 



