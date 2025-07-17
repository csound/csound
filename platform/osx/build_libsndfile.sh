#!/bin/bash
set -e

# Configuration
TARGET_PLATFORM="${1:-macos}"  # default to macOS if nothing is passed
PROJECT_NAME="libsndfile_build" # Name for our project folder
LIBOGGVERSION="1.3.1"
LIBVORBISVERSION="1.3.5"
OPUSVERSION="1.4"
FLACVERSION="1.4.3"
LAMEVERSION="3.100"
LIBSNDFILEVERSION="1.2.2"
SDKVERSION="12.3"
MINOSVERSION="11.0"

# Create project directory structure
echo "Creating project directory structure..."
PROJECT_ROOT="$(pwd)/${PROJECT_NAME}"
BUILDDIR="${PROJECT_ROOT}/build"
SRCDIR="${BUILDDIR}/src"
INTERDIR="${BUILDDIR}/built"
OUTPUTDIR="${PROJECT_ROOT}/dependencies"

mkdir -p "${PROJECT_ROOT}"
mkdir -p "${OUTPUTDIR}/include" "${OUTPUTDIR}/lib" "${SRCDIR}" "${INTERDIR}"

echo "Project will be built in: ${PROJECT_ROOT}"
cd "${PROJECT_ROOT}"

# Architectures per platform
if [[ "$TARGET_PLATFORM" == "ios" ]]; then
  ARCHS=("armv7" "armv7s" "arm64" "x86_64")  # iOS architectures
  PLATFORM_IOS="true"
  CMAKE_IOS_OPTIONS="-DCMAKE_SYSTEM_NAME=iOS"
else
  ARCHS=("x86_64" "arm64")  # macOS architectures
  PLATFORM_IOS="false"
  CMAKE_IOS_OPTIONS=""
fi

# Download sources
echo "Downloading source code..."
cd "$SRCDIR"
curl -sL "http://downloads.xiph.org/releases/ogg/libogg-${LIBOGGVERSION}.tar.xz" | tar -xJ
curl -sL "http://downloads.xiph.org/releases/vorbis/libvorbis-${LIBVORBISVERSION}.tar.xz" | tar -xJ
curl -sL "https://downloads.xiph.org/releases/opus/opus-${OPUSVERSION}.tar.gz" | tar -xz
curl -sL "https://downloads.xiph.org/releases/flac/flac-${FLACVERSION}.tar.xz" | tar -xJ
curl -sL "https://downloads.sourceforge.net/project/lame/lame/${LAMEVERSION}/lame-${LAMEVERSION}.tar.gz" | tar xz -C "${SRCDIR}"
git clone --depth 1 --branch ${LIBSNDFILEVERSION} https://github.com/libsndfile/libsndfile.git

LIBS=("libogg-${LIBOGGVERSION}" "libvorbis-${LIBVORBISVERSION}" "opus-${OPUSVERSION}" "flac-${FLACVERSION}" "lame-${LAMEVERSION}")
OUTPUT_LIBS=("libogg.a" "libvorbis.a" "libvorbisenc.a" "libvorbisfile.a" "libopus.a" "libFLAC.a" "libmp3lame.a")

# Build dependencies
for LIB in "${LIBS[@]}"; do
  echo "Building $LIB..."

  for ARCH in "${ARCHS[@]}"; do
    if [[ "$PLATFORM_IOS" == "true" ]]; then
      if [[ "$ARCH" == "x86_64" ]]; then
        PLATFORM="iPhoneSimulator"
        HOST="x86_64-apple-darwin"
      else
        PLATFORM="iPhoneOS"
        if [[ "$ARCH" == "arm64" ]]; then
          HOST="aarch64-apple-darwin"
        else
          HOST="arm-apple-darwin"
        fi
      fi
    else
      PLATFORM="MacOSX"
      if [[ "$ARCH" == "x86_64" ]]; then
        HOST="x86_64-apple-darwin"
      else
        HOST="aarch64-apple-darwin"
      fi
    fi

    ARCHDIR="${INTERDIR}/${PLATFORM}${SDKVERSION}-${ARCH}.sdk"
    mkdir -p "$ARCHDIR"

    cd "${SRCDIR}/${LIB}"
    
    # Skip distclean if Makefile doesn't exist
    if [[ -f "Makefile" ]]; then
      make distclean || true
    fi

    # Special handling for LAME
    if [[ "$LIB" == "lame-${LAMEVERSION}" ]]; then
      # Update config.sub for modern architectures
      CONFIG_SUB_URL="https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.sub"
      curl -s "${CONFIG_SUB_URL}" -o "${SRCDIR}/${LIB}/config.sub" || {
        echo "Warning: Failed to download updated config.sub, using manual patch"
        sed -i '' -e 's/arm \*-/arm* | aarch64* | arm64*-/' \
                  -e 's/armeb \*-/armeb* | aarch64* | arm64*-/' \
                  "${SRCDIR}/${LIB}/config.sub"
      }
      chmod +x "${SRCDIR}/${LIB}/config.sub"
      
      # LAME specific configure
      "${SRCDIR}/${LIB}/configure" \
        --prefix="$ARCHDIR" \
        --disable-shared \
        --enable-static \
        --enable-nasm \
        --disable-decoder \
        --disable-frontend \
        --host="${HOST}" \
        CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}" \
        LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}"
    else
      # Standard configure for other libraries
      if [[ "$LIB" == "libvorbis-${LIBVORBISVERSION}" ]]; then
        sed -i '' 's/-force_cpusubtype_ALL//g' configure
        sed -i '' 's/ test_programs//g' Makefile.in
        sed -i '' 's/^SUBDIRS = .*$/SUBDIRS = /' Makefile.in
      fi

      if [[ "$LIB" == "opus-${OPUSVERSION}" ]]; then
        EXTRA_FLAGS="--disable-extra-programs"
      else
        EXTRA_FLAGS=""
      fi

      ./configure --disable-shared --enable-static --with-pic \
        --host="$HOST" \
        --prefix="$ARCHDIR" \
        CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}" \
        CXXFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}" \
        LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}" \
        $EXTRA_FLAGS
    fi

    # Build and install
    if [[ "$LIB" == "libvorbis-${LIBVORBISVERSION}" ]]; then
      cd "${SRCDIR}/${LIB}/lib"
      make -j$(sysctl -n hw.logicalcpu)
      cd ..
      make -j$(sysctl -n hw.logicalcpu) install-exec
      make -j$(sysctl -n hw.logicalcpu) install-data
    else
      make -j$(sysctl -n hw.logicalcpu)
      make install
    fi
    
    # Skip clean if Makefile doesn't exist
    if [[ -f "Makefile" ]]; then
      make clean || true
    fi
  done
done

# [Rest of the script remains the same...]

# Create universal .a libraries for dependencies
for OUTPUT_LIB in "${OUTPUT_LIBS[@]}"; do
  echo "Creating universal: $OUTPUT_LIB"
  INPUTS=()
  for ARCH in "${ARCHS[@]}"; do
    if [[ "$PLATFORM_IOS" == "true" ]]; then
      if [[ "$ARCH" == "x86_64" ]]; then
        PLATFORM_NAME="iPhoneSimulator"
      else
        PLATFORM_NAME="iPhoneOS"
      fi
    else
      PLATFORM_NAME="MacOSX"
    fi
    LIB_PATH="${INTERDIR}/${PLATFORM_NAME}${SDKVERSION}-${ARCH}.sdk/lib/${OUTPUT_LIB}"
    if [[ -f "$LIB_PATH" ]]; then
      INPUTS+=("$LIB_PATH")
    fi
  done

  if [[ ${#INPUTS[@]} -gt 0 ]]; then
    lipo -create "${INPUTS[@]}" -output "${OUTPUTDIR}/lib/${OUTPUT_LIB}"
  else
    echo "Warning: $OUTPUT_LIB not found for any archs."
  fi
done

# Copy headers once
for ARCH in "${ARCHS[@]}"; do
  if [[ "$PLATFORM_IOS" == "true" ]]; then
    if [[ "$ARCH" == "x86_64" ]]; then
      PLATFORM_NAME="iPhoneSimulator"
    else
      PLATFORM_NAME="iPhoneOS"
    fi
  else
    PLATFORM_NAME="MacOSX"
  fi
  INCDIR="${INTERDIR}/${PLATFORM_NAME}${SDKVERSION}-${ARCH}.sdk/include"
  if [[ -d "$INCDIR" ]]; then
    cp -R "$INCDIR/"* "$OUTPUTDIR/include/"
    break
  fi
done

# Build libsndfile with CMake - single universal build
echo "Building universal libsndfile with CMake..."
cd "${SRCDIR}/libsndfile"

# Prepare architectures string for CMake
IFS=";" eval 'ARCH_STRING="${ARCHS[*]}"'

# Configure CMake with all architectures at once
cmake -B "${BUILDDIR}/libsndfile-universal" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_PROGRAMS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF \
  -DINSTALL_PKGCONFIG_MODULE=OFF \
  -DENABLE_EXTERNAL_LIBS=OFF \
  -DENABLE_MP3=ON \
  -DCMAKE_INSTALL_PREFIX="${OUTPUTDIR}" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MINOSVERSION}" \
  -DCMAKE_OSX_ARCHITECTURES="${ARCH_STRING}" \
  -DOGG_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DOGG_LIBRARY="${OUTPUTDIR}/lib/libogg.a" \
  -DVORBIS_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DVORBIS_LIBRARY="${OUTPUTDIR}/lib/libvorbis.a" \
  -DVORBISENC_LIBRARY="${OUTPUTDIR}/lib/libvorbisenc.a" \
  -DFLAC_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DFLAC_LIBRARY="${OUTPUTDIR}/lib/libFLAC.a" \
  -DOPUS_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DOPUS_LIBRARY="${OUTPUTDIR}/lib/libopus.a" \
  -DMP3_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DMP3_LIBRARY="${OUTPUTDIR}/lib/libmp3lame.a" \
  "${CMAKE_IOS_OPTIONS}"

# Build and install
cmake --build "${BUILDDIR}/libsndfile-universal" --config Release --parallel $(sysctl -n hw.logicalcpu)
cmake --install "${BUILDDIR}/libsndfile-universal"

echo "✅ Build completed successfully in ${PROJECT_ROOT}"
echo "Libraries are available in: ${OUTPUTDIR}/lib"
echo "Headers are available in: ${OUTPUTDIR}/include"