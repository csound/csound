APP_ABI := armeabi armeabi-v7a
APP_CPPFLAGS += -fexceptions -frtti -std=c++11
APP_OPTIM := release
APP_PLATFORM := android-21
#APP_PLATFORM := android-19
# LuaJIT does not currently build 
# for Android arm with clang.
# Both LuaJIT and LuaCsound must be built with gcc,
# and LuaJIT must be linked statically with LuaCsound.
#APP_STL := c++_shared
#NDK_TOOLCHAIN_VERSION := clang
APP_STL := gnustl_static
#APP_STL := gnustl_shared
NDK_TOOLCHAIN_VERSION := 4.9
