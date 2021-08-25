set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)

if(${PORT} MATCHES "fluidsynth|glib|zlib|pcre|libffi|gettext|libiconv|portmidi|portaudio|liblo")
	set(VCPKG_LIBRARY_LINKAGE dynamic)
else()
	set(VCPKG_LIBRARY_LINKAGE static)
endif()
