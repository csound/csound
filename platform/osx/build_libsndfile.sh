#!/bin/bash
set -x

# Configuration
TARGET_PLATFORM="${1:-macos}"  # default to macOS if nothing is passed
PROJECT_NAME="libsndfile_build" # Name for our project folder
LIBOGGVERSION="1.3.1"
LIBVORBISVERSION="1.3.5"
MPG123_VERSION="1.33.0"
MPG123_TARBALL="mpg123-${MPG123_VERSION}.tar.bz2"
MPG123_DIR="mpg123-${MPG123_VERSION}"
OPUSVERSION="1.4"
FLACVERSION="1.4.3"
LAMEVERSION="3.100"
LAME_TARBALL="lame-${LAMEVERSION}.tar.gz"
LIBSNDFILEVERSION="1.2.2"
MINOSVERSION="11.0"

# Auto-detect SDK version
SDKVERSION=$(xcrun --sdk macosx --show-sdk-version)

# Clean up previous build if exists
echo "Cleaning up previous build (if any)..."
rm -rf "${PROJECT_NAME}"

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
  # On Apple Silicon, build arm64 first for better performance
  if [[ $(uname -m) == "arm64" ]]; then
    ARCHS=("arm64" "x86_64")
  else
    ARCHS=("x86_64" "arm64")
  fi
  PLATFORM_IOS="false"
  CMAKE_IOS_OPTIONS=""
fi

# Download sources
echo "Downloading source code..."
cd "$SRCDIR"
curl -sL "http://downloads.xiph.org/releases/ogg/libogg-${LIBOGGVERSION}.tar.xz" | tar -xJ
echo "Downloading vorbis..."
curl -sL "http://downloads.xiph.org/releases/vorbis/libvorbis-${LIBVORBISVERSION}.tar.xz" | tar -xJ
echo "Downloading opus..."
curl -sL "https://downloads.xiph.org/releases/opus/opus-${OPUSVERSION}.tar.gz" | tar -xz
# Always delete and re-extract FLAC source to avoid cross-arch contamination
rm -rf "flac-${FLACVERSION}"
echo "Downloading flac..."
curl -sL "https://downloads.xiph.org/releases/flac/flac-${FLACVERSION}.tar.xz" | tar -xJ
echo "Downloading lame..."
curl -sL -o "${LAME_TARBALL}" "https://downloads.sourceforge.net/project/lame/lame/${LAMEVERSION}/lame-${LAMEVERSION}.tar.gz" 
tar -xzf "${LAME_TARBALL}"
git clone --depth 1 --branch ${LIBSNDFILEVERSION} https://github.com/libsndfile/libsndfile.git
# Download mpg123 official release tarball
echo "Downloading mpg123..."
curl -sL -o "${MPG123_TARBALL}" "https://www.mpg123.de/download/mpg123-1.33.0.tar.bz2"
tar -xjf "${MPG123_TARBALL}"

# Patch libsndfile CMakeLists.txt to use direct library paths for Vorbis
LIBSNDFILE_CMAKELISTS="${SRCDIR}/libsndfile/CMakeLists.txt"
sed -i.bak \
  -e 's/Vorbis::vorbisenc/${VORBISENC_LIBRARY}/g' \
  -e 's/Vorbis::vorbis/${VORBIS_LIBRARY}/g' \
  "$LIBSNDFILE_CMAKELISTS"

# Verify downloads
LIBS=("libogg-${LIBOGGVERSION}" "libvorbis-${LIBVORBISVERSION}" "opus-${OPUSVERSION}" "flac-${FLACVERSION}" "lame-${LAMEVERSION}")
OUTPUT_LIBS=("libogg.a" "libvorbis.a" "libvorbisenc.a" "libvorbisfile.a" "libopus.a" "libFLAC.a" "libmp3lame.a")

for LIB in "${LIBS[@]}"; do
  if [[ ! -d "${SRCDIR}/${LIB}" ]]; then
    echo "Error: Failed to download/extract ${LIB}"
    exit 1
  fi
done

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
    if [[ "$LIB" == "flac-${FLACVERSION}" ]]; then
      # Build FLAC with CMake for universal (arm64 + x86_64) static library
      FLAC_BUILD_DIR="${SRCDIR}/${LIB}/cmake-universal"
      rm -rf "$FLAC_BUILD_DIR"
      mkdir -p "$FLAC_BUILD_DIR"
      cd "$FLAC_BUILD_DIR"
      rm -rf "${SRCDIR}/flac-${FLACVERSION}/microbench"
      sed -i.bak '/add_subdirectory.*microbench/d' "${SRCDIR}/flac-${FLACVERSION}/CMakeLists.txt"
      cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_PROGRAMS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_MICROBENCH=OFF \
        -DINSTALL_MANPAGES=OFF \
        -DCMAKE_INSTALL_PREFIX="${OUTPUTDIR}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MINOSVERSION}" \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DOGG_INCLUDE_DIR="${OUTPUTDIR}/include" \
        -DOGG_LIBRARY="${OUTPUTDIR}/lib/libogg.a"
      cmake --build . --config Release --parallel $(sysctl -n hw.logicalcpu)
      cmake --install .
      cd ../..
      continue
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
        --host="$HOST" \
        CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -I${OUTPUTDIR}/include" \
        LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -L${OUTPUTDIR}/lib"
    elif [[ "$LIB" == "libvorbis-${LIBVORBISVERSION}" ]]; then
      # Verify ogg headers exist before building vorbis
      if [[ ! -f "${OUTPUTDIR}/include/ogg/ogg.h" ]]; then
        echo "Error: Missing ogg.h - libogg build may have failed"
        exit 1
      fi

      # Special handling for libvorbis
      sed -i '' 's/-force_cpusubtype_ALL//g' configure
      sed -i '' 's/ test_programs//g' Makefile.in
      sed -i '' 's/^SUBDIRS = .*$/SUBDIRS = /' Makefile.in
      
      export PKG_CONFIG_PATH="$ARCHDIR/lib/pkgconfig"
      ./configure \
        --prefix="$ARCHDIR" \
        --disable-shared \
        --enable-static \
        --with-pic \
        --host="$HOST" \
        CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -I$ARCHDIR/include -I$ARCHDIR/include/ogg" \
        CPPFLAGS="-I$ARCHDIR/include -I$ARCHDIR/include/ogg" \
        LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -L$ARCHDIR/lib" \
        PKG_CONFIG_PATH="$ARCHDIR/lib/pkgconfig" \
        PKG_CONFIG_SYSROOT_DIR="/nonexistent" \
        --with-ogg-libraries="$ARCHDIR/lib" \
        --with-ogg-includes="$ARCHDIR/include"
    else
      # Standard configure for other libraries
      if [[ "$LIB" == "opus-${OPUSVERSION}" ]]; then
        EXTRA_FLAGS="--disable-extra-programs"
      else
        EXTRA_FLAGS=""
      fi

      ./configure \
        --prefix="$ARCHDIR" \
        --disable-shared \
        --enable-static \
        --with-pic \
        --host="$HOST" \
        CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -I${OUTPUTDIR}/include" \
        LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION} -L${OUTPUTDIR}/lib" \
        PKG_CONFIG_PATH="${OUTPUTDIR}/lib/pkgconfig" \
        PKG_CONFIG_SYSROOT_DIR="/nonexistent" \
        ${EXTRA_FLAGS}
    fi

    # Build and install
    if [[ "$LIB" == "libvorbis-${LIBVORBISVERSION}" ]]; then
      cd "${SRCDIR}/${LIB}/lib"
      make -j$(sysctl -n hw.logicalcpu)
      # Copy static libraries BEFORE install/clean
      for LIBNAME in libvorbis.a libvorbisenc.a libvorbisfile.a; do
        if [[ -f ".libs/$LIBNAME" ]]; then
          echo "Copying $LIBNAME from .libs to $ARCHDIR/lib"
          cp ".libs/$LIBNAME" "$ARCHDIR/lib/"
        elif [[ -f "$LIBNAME" ]]; then
          echo "Copying $LIBNAME from current dir to $ARCHDIR/lib"
          cp "$LIBNAME" "$ARCHDIR/lib/"
        fi
      done
      # Copy Vorbis headers to output include directory (only once)
      if [[ -d "$ARCHDIR/include/vorbis" ]]; then
        cp -R "$ARCHDIR/include/vorbis" "$OUTPUTDIR/include/"
      elif [[ -d "${SRCDIR}/libvorbis-${LIBVORBISVERSION}/include/vorbis" ]]; then
        cp -R "${SRCDIR}/libvorbis-${LIBVORBISVERSION}/include/vorbis" "$OUTPUTDIR/include/"
      fi
      cd ..
      make -j$(sysctl -n hw.logicalcpu) install-exec
      make -j$(sysctl -n hw.logicalcpu) install-data
    else
      make -j$(sysctl -n hw.logicalcpu)
      make install
      
      # Copy ogg headers immediately after install
      if [[ "$LIB" == "libogg-${LIBOGGVERSION}" ]]; then
        cp -R "${ARCHDIR}/include/"* "${OUTPUTDIR}/include/"
      fi
    fi
    
    # Skip clean if Makefile doesn't exist
    if [[ -f "Makefile" ]]; then
      make clean || true
    fi
  done
done

# Build mpg123 for each architecture
MPG123_SRCDIR="${SRCDIR}/${MPG123_DIR}"
echo "Building mpg123..."
for ARCH in "${ARCHS[@]}"; do
  if [[ "$ARCH" == "x86_64" ]]; then
    HOST="x86_64-apple-darwin"
  else
    HOST="aarch64-apple-darwin"
  fi
  ARCHDIR="${INTERDIR}/MacOSX${SDKVERSION}-${ARCH}.sdk"
  mkdir -p "$ARCHDIR"
  cd "$MPG123_SRCDIR"
  if [[ -f Makefile ]]; then
    make distclean || true
  fi
  ./configure \
    --prefix="$ARCHDIR" \
    --disable-shared \
    --enable-static \
    --with-cpu=generic_fpu \
    --host="$HOST" \
    CFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}" \
    LDFLAGS="-arch $ARCH -mmacosx-version-min=${MINOSVERSION}"
  make -j$(sysctl -n hw.logicalcpu)
  make install
done

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
# Create universal libmpg123.a
echo "Creating universal libmpg123.a"
INPUTS=()
for ARCH in "${ARCHS[@]}"; do
  LIB_PATH="${INTERDIR}/MacOSX${SDKVERSION}-${ARCH}.sdk/lib/libmpg123.a"
  if [[ -f "$LIB_PATH" ]]; then
    INPUTS+=("$LIB_PATH")
  fi
done
if [[ ${#INPUTS[@]} -gt 0 ]]; then
  lipo -create "${INPUTS[@]}" -output "${OUTPUTDIR}/lib/libmpg123.a"
else
  echo "Warning: libmpg123.a not found for any archs."
fi
# Copy mpg123 headers
for ARCH in "${ARCHS[@]}"; do
  INCDIR="${INTERDIR}/MacOSX${SDKVERSION}-${ARCH}.sdk/include"
  if [[ -d "$INCDIR" ]]; then
    cp -R "$INCDIR/"* "$OUTPUTDIR/include/" 2>/dev/null || true
    break
  fi
done

# Copy headers once (if not already copied)
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
    cp -R "$INCDIR/"* "$OUTPUTDIR/include/" 2>/dev/null || true
    break
  fi
done

# Build libsndfile with CMake - single universal build
echo "Building universal libsndfile with CMake..."
cd "${SRCDIR}/libsndfile"

# Prepare architectures string for CMake
IFS=";" eval 'ARCH_STRING="${ARCHS[*]}"'

# Configure CMake with all architectures at once
cmake -B "${BUILDDIR}/libsndfile-universal" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_PROGRAMS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF \
  -DINSTALL_PKGCONFIG_MODULE=OFF \
  -DENABLE_EXTERNAL_LIBS=ON \
  -DENABLE_MP3=ON \
  -DENABLE_MPEG=ON \
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
  -DMPG123_INCLUDE_DIR="${OUTPUTDIR}/include" \
  -DMPG123_LIBRARY="${OUTPUTDIR}/lib/libmpg123.a" \
  -DCMAKE_FIND_ROOT_PATH="${OUTPUTDIR}" \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
  -DCMAKE_IGNORE_PATH="/usr/local/lib;/usr/lib" \
  "${CMAKE_IOS_OPTIONS}"

# Build and install
cmake --build "${BUILDDIR}/libsndfile-universal" --config Release --parallel $(sysctl -n hw.logicalcpu)
cmake --install "${BUILDDIR}/libsndfile-universal"

echo "Build completed successfully in ${PROJECT_ROOT}"
echo "Libraries are available in: ${OUTPUTDIR}/lib"
echo "Headers are available in: ${OUTPUTDIR}/include"
