AndroidCsound C++ app requirements

1. Libraries go into jniLibs under Project/app/src/main
2. Cmake target_link_directories is used to find them.
3. Header files added to Project/app/src/main/cpp/csound
4. Add float.h and version.h from Android/CsoundAndroid/jni sources



