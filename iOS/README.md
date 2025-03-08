Csound for iOS
----------

Building:
------
CMake and XCode are required. First build libsndfile

sh build_libsndfile.sh

Then build the Csound xcframework

sh build.sh

This will install the frameworks under Csound-For-iOS. The xcode projects in that directory contain examples for Obj-C and Swift.