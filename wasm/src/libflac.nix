{
  pkgs,
  pkgsWasm,
  stdenvWasm,
}: let
  lib = pkgs.lib;
  libvorbis = pkgs.callPackage ./libvorbis.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libogg = pkgs.callPackage ./libogg.nix {inherit pkgs pkgsWasm stdenvWasm;};
in
  stdenvWasm.mkDerivation rec {
    name = "libflac";
    src = pkgs.flac.src;
    nativeBuildInputs = [
      pkgs.cmake
      pkgs.pkg-config
    ];

    postPatch = ''
        echo "Prepending WASI chown() stub to libFLAC metadata_iterators.c"
        tmpfile=$(mktemp)
        cat > "$tmpfile" <<'EOF'
      /* ---------------------------------------------------------------------
       * WASI / WebAssembly stub for chown()
       * ---------------------------------------------------------------------
       * WASI and other WebAssembly environments have no user/group ownership
       * system calls, so we provide a harmless no-op implementation to allow
       * code expecting POSIX chown() to compile and link successfully.
       */
      #if defined(__wasm__) || defined(__wasi__)
      #  include <unistd.h>
      #  include <errno.h>
      int chown(const char *path, uid_t owner, gid_t group) {
          (void)path; (void)owner; (void)group;
          errno = ENOSYS; /* Operation not supported */
          return -1;      /* or return 0 to silently succeed */
      }
      #endif /* __wasm__ || __wasi__ */

      EOF
        cat src/libFLAC/metadata_iterators.c >> "$tmpfile"
        mv "$tmpfile" src/libFLAC/metadata_iterators.c
    '';

    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=RELEASE"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_PROGRAMS=OFF"
      "-DBUILD_CXXLIBS=OFF"
      "-DBUILD_DOCS=OFF"
      "-DBUILD_EXAMPLES=OFF"
      "-DBUILD_TESTING=OFF"
      "-DINSTALL_MANPAGES=OFF"
      "-DENABLE_MULTITHREADING=OFF"
      "-DWITH_FORTIFY_SOURCE=OFF"
      "-DWITH_STACK_PROTECTOR=OFF"
      "-DOGG_INCLUDE_DIR=${libogg}/include"
      "-DOGG_LIBRARY=${libogg}/lib"
    ];
  }
