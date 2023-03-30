APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_CPPFLAGS += -fexceptions -frtti
APP_OPTIM := release
APP_PLATFORM := android-21
#APP_STL := gnustl_shared
#NDK_TOOLCHAIN_VERSION := 4.9
APP_STL := c++_shared
NDK_TOOLCHAIN_VERSION := clang
