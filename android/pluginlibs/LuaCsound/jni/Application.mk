APP_ABI := armeabi armeabi-v7a
APP_CPPFLAGS += -fexceptions -frtti -std=c++11
APP_OPTIM := release
APP_PLATFORM := 19
# The following two lines are required because luajit does not currently build 
# for Android arm with clang.
APP_STL := gnustl_static
NDK_TOOLCHAIN_VERSION := 4.9
