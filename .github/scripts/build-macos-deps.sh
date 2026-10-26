#!/usr/bin/env bash
set -euo pipefail

source platform/osx/dependencies.env

checkout_dependency() {
  local directory=$1 repository=$2 revision=$3
  git init "$directory"
  git -C "$directory" fetch --depth 1 "$repository" "$revision"
  git -C "$directory" checkout --detach FETCH_HEAD
}

(cd platform/osx && bash -e -o pipefail build_libsndfile.sh)

checkout_dependency portmidi https://github.com/PortMidi/portmidi.git "$PORTMIDI_REVISION"
checkout_dependency portaudio https://github.com/PortAudio/portaudio.git "$PORTAUDIO_REVISION"
checkout_dependency libsamplerate https://github.com/libsndfile/libsamplerate.git "$LIBSAMPLERATE_REVISION"
checkout_dependency liblo https://github.com/radarsat1/liblo.git "$LIBLO_REVISION"

for library in portmidi portaudio libsamplerate liblo; do
  source_dir=$library
  install_dir=${library}_install
  if [[ "$library" == libsamplerate ]]; then
    install_dir=samplerate_install
  elif [[ "$library" == liblo ]]; then
    source_dir=liblo/cmake
  fi
  cmake -S "$source_dir" -B "$library/build" -G Ninja \
    -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64' \
    -DCMAKE_INSTALL_PREFIX="$PWD/$install_dir"
  cmake --build "$library/build"
  cmake --install "$library/build"
done

# Reject incomplete universal libraries before saving the cache.
while IFS= read -r -d '' library; do
  lipo -verify_arch arm64 x86_64 "$library"
done < <(find platform/osx/libsndfile_build/dependencies portmidi_install \
  portaudio_install samplerate_install liblo_install \
  -type f \( -name '*.a' -o -name '*.dylib' \) -print0)
