APP_ALLOW_MISSING_DEPS=true
APP_ABI := x86 x86_64 armeabi-v7a arm64-v8a #
APP_CPPFLAGS += -fexceptions -frtti
APP_OPTIM := release
APP_PLATFORM := android-29
#APP_STL := gnustl_shared
#NDK_TOOLCHAIN_VERSION := 4.9
APP_STL := c++_shared
NDK_TOOLCHAIN_VERSION := clang

