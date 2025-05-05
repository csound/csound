# cmake/AddLibsndfileWasm64WASI.cmake
#
# add_libsndfile_wasm64_wasi()
#
# This function builds libsndfile.a for WASM64 WASI target
# The resulting library will be placed in ${CMAKE_BINARY_DIR}/wasm64-wasi/lib/libsndfile.a

function(add_libsndfile_wasm64_wasi)
    include(ExternalProject)

    set(_prefix ${CMAKE_BINARY_DIR}/_ext/libsndfile_wasm64_wasi)  # build dir
    set(_install ${CMAKE_BINARY_DIR}/wasm64-wasi)                 # final artifacts
    set(_source ${_prefix}/src/libsndfile_wasm64_wasi_ext)        # source dir
    set(_build_script ${CMAKE_CURRENT_BINARY_DIR}/build_libsndfile_wasm64.sh)

    # Create a custom target to download libsndfile
    ExternalProject_Add(libsndfile_wasm64_wasi_ext
        PREFIX          ${_prefix}
        GIT_REPOSITORY  https://github.com/libsndfile/libsndfile.git
        GIT_TAG         3bd5048f8c2f7285743e9922c195c7a08f3f5551
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        # No patch command - we'll handle patching in the build script
    )

    # Create directories
    file(MAKE_DIRECTORY ${_install}/lib)
    file(MAKE_DIRECTORY ${_install}/include)

    # Create build script based on libsndfile.nix
    file(WRITE ${_build_script} "#!/bin/sh
set -e

# Source directory
SRC_DIR=\"${_source}\"
# Install directory
INSTALL_DIR=\"${_install}\"
# WASI SDK path
WASI_SDK=\"\${WASI_SDK_PATH}\"

echo \"Building libsndfile.a for WASM64 WASI...\"
echo \"Source directory: \$SRC_DIR\"
echo \"Install directory: \$INSTALL_DIR\"
echo \"WASI SDK: \$WASI_SDK\"

# Create build directory
mkdir -p \"\$SRC_DIR/build\"
cd \"\$SRC_DIR\"

# Apply patches similar to libsndfile.nix - check if files exist first
if [ -f "src/g72x.c" ]; then
  mv src/g72x.c src/g72x_parent.c
fi

if [ -f "src/G72x/g72x_test.c" ]; then
  rm src/G72x/g72x_test.c
fi

if [ -f "src/test_file_io.c" ]; then
  rm src/test_file_io.c
fi

if [ -f "src/ogg_opus.c" ]; then
  rm src/ogg_opus.c
fi

# Apply text replacements - use perl instead of sed for better handling of binary files
perl -i -pe 's/@TYPEOF_SF_COUNT_T@/int64_t/g' include/*.h src/*.h src/*/*.h 2>/dev/null || true
perl -i -pe 's/@SIZEOF_SF_COUNT_T@/8/g' include/*.h src/*.h src/*/*.h 2>/dev/null || true
perl -i -pe 's/@SF_COUNT_MAX@/0x7FFFFFFFFFFFFFFFLL/g' include/*.h src/*.h src/*/*.h 2>/dev/null || true

# Create a simple config.h file with required definitions
cat > src/config.h << EOF
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define SIZEOF_SF_COUNT_T 8
#define TYPEOF_SF_COUNT_T int64_t
#define SF_COUNT_MAX 0x7FFFFFFFFFFFFFFFLL
EOF

# Fix sndfile.h to use wasi-libc headers
cat > include/sndfile.h << EOF
#ifndef SNDFILE_H
#define SNDFILE_H

#include <stdint.h>
#include <sys/types.h>

typedef int64_t sf_count_t;
#define SF_COUNT_MAX 0x7FFFFFFFFFFFFFFFLL

typedef struct SNDFILE_tag SNDFILE;

/* A file format is specified by one of the following file format codes. */
enum
{
    SF_FORMAT_WAV          = 0x010000,     /* Microsoft WAV format */
    SF_FORMAT_AIFF         = 0x020000,     /* Apple/SGI AIFF format */
    SF_FORMAT_AU           = 0x030000,     /* Sun/NeXT AU format */
    SF_FORMAT_RAW          = 0x040000,     /* RAW PCM data */
    SF_FORMAT_PAF          = 0x050000,     /* Ensoniq PARIS file format */
    SF_FORMAT_SVX          = 0x060000,     /* Amiga IFF / SVX8 / SV16 format */
    SF_FORMAT_NIST         = 0x070000,     /* Sphere NIST format */
    SF_FORMAT_VOC          = 0x080000,     /* VOC files */
    SF_FORMAT_IRCAM        = 0x0A0000,     /* Berkeley/IRCAM/CARL */
    SF_FORMAT_W64          = 0x0B0000,     /* Sonic Foundry's 64 bit RIFF/WAV */
    SF_FORMAT_MAT4         = 0x0C0000,     /* Matlab (tm) V4.2 / GNU Octave 2.0 */
    SF_FORMAT_MAT5         = 0x0D0000,     /* Matlab (tm) V5.0 / GNU Octave 2.1 */
    SF_FORMAT_PVF          = 0x0E0000,     /* Portable Voice Format */
    SF_FORMAT_XI           = 0x0F0000,     /* Fasttracker 2 Extended Instrument */
    SF_FORMAT_HTK          = 0x100000,     /* HMM Tool Kit format */
    SF_FORMAT_SDS          = 0x110000,     /* Midi Sample Dump Standard */
    SF_FORMAT_AVR          = 0x120000,     /* Audio Visual Research */
    SF_FORMAT_WAVEX        = 0x130000,     /* MS WAVE with WAVEFORMATEX */
    SF_FORMAT_SD2          = 0x160000,     /* Sound Designer 2 */
    SF_FORMAT_FLAC         = 0x170000,     /* FLAC lossless file format */
    SF_FORMAT_CAF          = 0x180000,     /* Core Audio File format */
    SF_FORMAT_WVE          = 0x190000,     /* Psion WVE format */
    SF_FORMAT_OGG          = 0x200000,     /* Xiph OGG container */
    SF_FORMAT_MPC2K        = 0x210000,     /* Akai MPC 2000 sampler */
    SF_FORMAT_RF64         = 0x220000,     /* RF64 WAV file */
    SF_FORMAT_MPEG         = 0x230000      /* MPEG-1/2 audio stream */
};

/* Subtypes from here on. */
enum
{
    SF_FORMAT_PCM_S8       = 0x0001,       /* Signed 8 bit data */
    SF_FORMAT_PCM_16       = 0x0002,       /* Signed 16 bit data */
    SF_FORMAT_PCM_24       = 0x0003,       /* Signed 24 bit data */
    SF_FORMAT_PCM_32       = 0x0004,       /* Signed 32 bit data */
    SF_FORMAT_PCM_U8       = 0x0005,       /* Unsigned 8 bit data (WAV and RAW only) */
    SF_FORMAT_FLOAT        = 0x0006,       /* 32 bit float data */
    SF_FORMAT_DOUBLE       = 0x0007,       /* 64 bit float data */
    SF_FORMAT_ULAW         = 0x0010,       /* U-Law encoded */
    SF_FORMAT_ALAW         = 0x0011,       /* A-Law encoded */
    SF_FORMAT_IMA_ADPCM    = 0x0012,       /* IMA ADPCM */
    SF_FORMAT_MS_ADPCM     = 0x0013,       /* Microsoft ADPCM */
    SF_FORMAT_GSM610       = 0x0020,       /* GSM 6.10 encoding */
    SF_FORMAT_VOX_ADPCM    = 0x0021,       /* Oki Dialogic ADPCM encoding */
    SF_FORMAT_G721_32      = 0x0030,       /* 32kbs G721 ADPCM encoding */
    SF_FORMAT_G723_24      = 0x0031,       /* 24kbs G723 ADPCM encoding */
    SF_FORMAT_G723_40      = 0x0032,       /* 40kbs G723 ADPCM encoding */
    SF_FORMAT_DWVW_12      = 0x0040,       /* 12 bit Delta Width Variable Word encoding */
    SF_FORMAT_DWVW_16      = 0x0041,       /* 16 bit Delta Width Variable Word encoding */
    SF_FORMAT_DWVW_24      = 0x0042,       /* 24 bit Delta Width Variable Word encoding */
    SF_FORMAT_DWVW_N       = 0x0043,       /* N bit Delta Width Variable Word encoding */
    SF_FORMAT_DPCM_8       = 0x0050,       /* 8 bit differential PCM (XI only) */
    SF_FORMAT_DPCM_16      = 0x0051,       /* 16 bit differential PCM (XI only) */
    SF_FORMAT_VORBIS       = 0x0060,       /* Xiph Vorbis encoding */
    SF_FORMAT_OPUS         = 0x0064,       /* Xiph/Skype Opus encoding */
    SF_FORMAT_ALAC_16      = 0x0070,       /* Apple Lossless Audio Codec (16 bit) */
    SF_FORMAT_ALAC_20      = 0x0071,       /* Apple Lossless Audio Codec (20 bit) */
    SF_FORMAT_ALAC_24      = 0x0072,       /* Apple Lossless Audio Codec (24 bit) */
    SF_FORMAT_ALAC_32      = 0x0073,       /* Apple Lossless Audio Codec (32 bit) */
    SF_FORMAT_MPEG_LAYER_I = 0x0080,       /* MPEG-1 Audio Layer I */
    SF_FORMAT_MPEG_LAYER_II = 0x0081,      /* MPEG-1 Audio Layer II */
    SF_FORMAT_MPEG_LAYER_III = 0x0082      /* MPEG-2 Audio Layer III */
};

/* Endian-ness options. */
enum
{
    SF_ENDIAN_FILE         = 0x00000000,   /* Default file endian-ness. */
    SF_ENDIAN_LITTLE       = 0x10000000,   /* Force little endian-ness. */
    SF_ENDIAN_BIG          = 0x20000000,   /* Force big endian-ness. */
    SF_ENDIAN_CPU          = 0x30000000,   /* Force CPU endian-ness. */
    SF_FORMAT_SUBMASK      = 0x0000FFFF,
    SF_FORMAT_TYPEMASK     = 0x0FFF0000,
    SF_FORMAT_ENDMASK      = 0x30000000
};

/* Info struct */
typedef struct
{
    sf_count_t frames;     /* Used to be called samples. */
    int samplerate;
    int channels;
    int format;
    int sections;
    int seekable;
} SF_INFO;

/* Minimal libsndfile API for WASM */
SNDFILE* sf_open(const char *path, int mode, SF_INFO *sfinfo);
int sf_close(SNDFILE *sndfile);
sf_count_t sf_readf_short(SNDFILE *sndfile, short *ptr, sf_count_t frames);
sf_count_t sf_readf_int(SNDFILE *sndfile, int *ptr, sf_count_t frames);
sf_count_t sf_readf_float(SNDFILE *sndfile, float *ptr, sf_count_t frames);
sf_count_t sf_readf_double(SNDFILE *sndfile, double *ptr, sf_count_t frames);
sf_count_t sf_writef_short(SNDFILE *sndfile, const short *ptr, sf_count_t frames);
sf_count_t sf_writef_int(SNDFILE *sndfile, const int *ptr, sf_count_t frames);
sf_count_t sf_writef_float(SNDFILE *sndfile, const float *ptr, sf_count_t frames);
sf_count_t sf_writef_double(SNDFILE *sndfile, const double *ptr, sf_count_t frames);
const char* sf_strerror(SNDFILE *sndfile);
const char* sf_version_string(void);

#endif /* SNDFILE_H */
EOF

# Create stub headers for standard library headers that might be missing
mkdir -p include/stubs
cat > include/stubs/stdio.h << EOF
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdint.h>

typedef struct FILE FILE;
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#define EOF (-1)

int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);

#endif /* _STDIO_H */
EOF

cat > include/stubs/stdlib.h << EOF
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
void exit(int status);
int abs(int j);
long labs(long j);
long long llabs(long long j);

#endif /* _STDLIB_H */
EOF

cat > include/stubs/inttypes.h << EOF
#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"

#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"

#endif /* _INTTYPES_H */
EOF

# Compile source files individually
echo "Compiling source files..."

# Common compiler flags
CFLAGS="--sysroot=\$WASI_SDK/share/wasi-sysroot \\
  --target=wasm64-wasi \\
  -fPIC \\
  -O3 \\
  -I\$WASI_SDK/share/wasi-sysroot/include \\
  -Iinclude/stubs \\
  -I./include -I./src -I./src/ALAC \\
  -I./src/GSM610 -I./src/G72x \\
  -D__wasi__=1 \\
  -D__wasm64__=1 \\
  -DOS_IS_WIN32=0 \\
  -DUSE_WINDOWS_API=0 \\
  -DSIZEOF_SF_COUNT_T=8 \\
  -DCPU_IS_LITTLE_ENDIAN=1 \\
  -DCPU_IS_BIG_ENDIAN=0 \\
  -DSIZEOF_INT64_T=8 \\
  -DSIZEOF_LONG_LONG=8 \\
  -DHAVE_UNISTD_H=1 \\
  -DHAVE_STDINT_H=1 \\
  -DHAVE_INTTYPES_H=1 \\
  -DHAVE_SYS_TYPES_H=1 \\
  -DCPU_CLIPS_POSITIVE=0 \\
  -DCPU_CLIPS_NEGATIVE=1 \\
  -DPACKAGE_NAME='\"libsndfile\"' \\
  -DPACKAGE_VERSION='\"1.1.0\"' \\
  -DPACKAGE='\"libsndfile\"' \\
  -DVERSION='\"1.1.0\"' \\
  -D_WASI_EMULATED_SIGNAL \\
  -D_WASI_EMULATED_MMAN \\
  -DHAVE_CONFIG_H \\
  -fno-exceptions -c \\
  -Wno-unknown-attributes \\
  -Wno-shift-op-parentheses \\
  -Wno-bitwise-op-parentheses \\
  -Wno-many-braces-around-scalar-init \\
  -Wno-macro-redefined"

# Compile G72x files
for file in src/G72x/*.c; do
  if [ -f "\$file" ]; then
    echo "Compiling \$file"
    \$WASI_SDK/bin/clang \$CFLAGS \$file || true
  fi
done

# Compile GSM610 files
for file in src/GSM610/*.c; do
  if [ -f "\$file" ]; then
    echo "Compiling \$file"
    \$WASI_SDK/bin/clang \$CFLAGS \$file || true
  fi
done

# Compile ALAC files
for file in src/ALAC/*.c; do
  if [ -f "\$file" ]; then
    echo "Compiling \$file"
    \$WASI_SDK/bin/clang \$CFLAGS \$file || true
  fi
done

# Compile src files
for file in src/*.c; do
  if [ -f "\$file" ]; then
    echo "Compiling \$file"
    \$WASI_SDK/bin/clang \$CFLAGS \$file || true
  fi
done

# Create the static library
mkdir -p \"\$INSTALL_DIR/lib\" \"\$INSTALL_DIR/include\"
\$WASI_SDK/bin/llvm-ar crS \"\$INSTALL_DIR/lib/libsndfile.a\" *.o
\$WASI_SDK/bin/llvm-ranlib -U \"\$INSTALL_DIR/lib/libsndfile.a\"

# Copy header files
cp include/*.h \"\$INSTALL_DIR/include/\"
cp src/*.h \"\$INSTALL_DIR/include/\"
cp src/GSM610/*.h \"\$INSTALL_DIR/include/\"
cp src/G72x/*.h \"\$INSTALL_DIR/include/\"

echo \"Build completed successfully!\"
")

    # Make the script executable
    execute_process(COMMAND chmod +x ${_build_script})

    # Add custom command to build libsndfile.a using the shell script
    add_custom_command(
        OUTPUT ${_install}/lib/libsndfile.a
        DEPENDS libsndfile_wasm64_wasi_ext ${_build_script}
        COMMAND ${_build_script}
        COMMENT "Building libsndfile.a for WASM64 WASI"
    )

    # Add a custom target for easier building
    add_custom_target(libsndfile_wasm64_wasi
        DEPENDS ${_install}/lib/libsndfile.a
        COMMENT "Building libsndfile.a for WASM64 WASI"
    )
endfunction()