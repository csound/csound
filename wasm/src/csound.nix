{ pkgs ? import <nixpkgs> { }, static ? false }:

let
  lib = pkgs.lib;
  wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
  wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
  wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;

  static-link = ''
    echo "Create libcsound.wasm standalone"
    ${wasi-sdk}/bin/wasm-ld --lto-O2 \
      --entry=_start --import-memory \
      ${
         pkgs.lib.concatMapStrings (x: " --export=" + x + " ")
         (with builtins; fromJSON (readFile ./exports.json))
       } \
      -L${wasi-sdk}/share/wasi-sysroot/lib/wasm32-wasi \
      -L${libsndfile}/lib -L${libflac}/lib -L${libogg}/lib -L${libvorbis}/lib \
      -L${liblame}/lib -L${libmpg123}/lib \
      -lc -lc++ -lc++abi -lrt -lutil -lxnet -lresolv -lc-printscan-long-double \
      -lflac -logg -lvorbis -llame -lsndfile -lmpg123 -lwasi-emulated-getpid \
      -lwasi-emulated-signal -lwasi-emulated-mman -lwasi-emulated-process-clocks \
      ${wasi-sdk}/share/wasi-sysroot/lib/wasm32-wasi/crt1.o \
       *.o -o csound.static.wasm
  '';

  dyn-link = ''
    echo "Create libcsound.wasm pie"
    ${wasi-sdk}/bin/wasm-ld --lto-O2 \
      -z stack-size=128 \
      --export=__data_end \
      --experimental-pic -pie --entry=_start \
      --import-table --import-memory \
      ${
         pkgs.lib.concatMapStrings (x: " --export=" + x + " ")
         (with builtins; fromJSON (readFile ./exports.json))
       } \
      -L${wasi-sdk}/share/wasi-sysroot/lib/wasm32-unknown-emscripten \
      -L${libsndfile}/lib -L${libflac}/lib -L${libogg}/lib -L${libvorbis}/lib \
      -L${liblame}/lib -L${libmpg123}/lib \
      -lc -lc++ -lc++abi -lrt -lutil -lxnet -lresolv -lc-printscan-long-double \
      -lflac -logg -lvorbis -llame -lsndfile -lmpg123 -lwasi-emulated-getpid \
      -lwasi-emulated-signal -lwasi-emulated-mman -lwasi-emulated-process-clocks \
      ${wasi-sdk}/share/wasi-sysroot/lib/wasm32-unknown-emscripten/crt1.o \
       *.o -o csound.dylib.wasm
  '';

  exports = with builtins; (fromJSON (readFile ./exports.json));
  patchClock = pkgs.writeTextFile {
    name = "patchClock";
    executable = true;
    destination = "/bin/patchClock";
    text = ''
      #!${pkgs.nodejs}/bin/node
      const myArgs = process.argv.slice(2);
      const myFile = myArgs[0];
      const fs = require('fs')
      fs.readFile(myFile, 'utf8', function (err,data) {
        if (err) { return console.log(err); }
        const regex = "\\/\\* find out CPU frequency based on.*" +
                      "initialise a timer structure \\*\\/";
        const replace = `static int getTimeResolution(void) { return 0; }
        int gettimeofday (struct timeval *__restrict, void *__restrict);
        static inline int_least64_t get_real_time(void) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          return ((int_least64_t) tv.tv_usec
            + (int_least64_t) ((uint32_t) tv.tv_sec * (uint64_t) 1000000));}
        clock_t clock (void);
        static inline int_least64_t get_CPU_time(void) {
          return ((int_least64_t) ((uint32_t) clock()));
        }`;
        const result = data.replace(new RegExp(regex, 'is'), replace);
        fs.writeFile(myFile, result, 'utf8', function (err) {
          if (err) return console.log(err);
        });
      });
    '';
  };

  libsndfile = pkgs.callPackage ./libsndfile.nix { inherit static; };
  libogg = pkgs.callPackage ./libogg.nix { inherit static; };
  libflac = pkgs.callPackage ./libflac.nix { inherit static; };
  libvorbis = pkgs.callPackage ./libvorbis.nix { inherit static; };
  liblame = pkgs.callPackage ./liblame.nix { inherit static; };
  libmpg123 = pkgs.callPackage ./libmpg123.nix { inherit static; };

  csoundSrc = builtins.path {
    path = ./. + "../../../";
    filter = path: type:
      ((builtins.match ".*/Engine.*" path != null ||
        builtins.match ".*/H.*" path != null ||
        builtins.match ".*/InOut.*" path != null ||
        builtins.match ".*/OOps.*" path != null ||
        builtins.match ".*/Opcodes.*" path != null ||
        builtins.match ".*/Top.*" path != null ||
        builtins.match ".*/include.*" path != null) &&
      (lib.strings.hasSuffix ".c" path ||
       lib.strings.hasSuffix ".cpp" path ||
       lib.strings.hasSuffix ".h" path ||
       lib.strings.hasSuffix ".h.in" path ||
       lib.strings.hasSuffix ".hpp" path ||
       lib.strings.hasSuffix ".hpp.in" path ||
       lib.strings.hasSuffix ".y" path ||
       lib.strings.hasSuffix ".lex" path ||
       type == "directory"));
  };

  preprocFlags = ''
    -DUSE_LIBSNDFILE=1 \
    -DGIT_HASH_VALUE=HEAD \
    -DUSE_DOUBLE=1 \
    -DLINUX=0 \
    -DO_NDELAY=O_NONBLOCK \
    -DHAVE_STRLCAT=1 \
    -D__thread='^-^' \
    -Wno-unknown-attributes \
    -Wno-shift-op-parentheses \
    -Wno-bitwise-op-parentheses \
    -Wno-many-braces-around-scalar-init \
    -Wno-macro-redefined \
  '';

  staticPreprocFlags = ''
    -DO_WRONLY='(0x10000000)' \
    -DO_CREAT='(__WASI_OFLAGS_CREAT << 12)' \
  '';

in pkgs.stdenvNoCC.mkDerivation rec {

  name = "csound-wasm";
  src = csoundSrc;

  buildInputs = [ pkgs.flex pkgs.bison ];

  dontStrip = true;
  AR = "${wasi-sdk}/bin/ar";
  CC = "${wasi-sdk}/bin/clang";
  CPP = "${wasi-sdk}/bin/clang-cpp";
  CXX = "${wasi-sdk}/bin/clang++";
  LD = "${wasi-sdk}/bin/wasm-ld";
  NM = "${wasi-sdk}/bin/nm";
  OBJCOPY = "${wasi-sdk}/bin/objcopy";
  RANLIB = "${wasi-sdk}/bin/ranlib";

  postPatch = ''
    # Experimental setjmp patching
    find ./ -type f -exec sed -i -e 's/#include <setjmp.h>//g' {} \;
    find ./ -type f -exec sed -i -e 's/csound->LongJmp(csound, retval);/return retval;/g' {} \;
    find ./ -type f -exec sed -i -e 's/csound->LongJmp(.*)/(void)0/g' {} \;
    find ./ -type f -exec sed -i -e 's/longjmp(.*)/(void)0/g' {} \;
    find ./ -type f -exec sed -i -e 's/jmp_buf/int/g' {} \;
    find ./ -type f -exec sed -i -e 's/setjmp(csound->exitjmp)/0/g' {} \;

    # find ./ -type f -exec sed -i -e 's/csound->Calloc/mcalloc/g' {} \;

    find ./ -type f -exec sed -i -e 's/HAVE_PTHREAD/FFS_NO_PTHREADS/g' {} \;
    find ./ -type f -exec sed -i -e 's/#ifdef LINUX/#ifdef _NOT_LINUX_/g' {} \;
    find ./ -type f -exec sed -i -e 's/if(LINUX)/if(_NOT_LINUX_)/g' {} \;
    find ./ -type f -exec sed -i -e 's/if (LINUX)/if(_NOT_LINUX_)/g' {} \;
    find ./ -type f -exec sed -i -e 's/defined(LINUX)/defined(_NOT_LINUX_)/g' {} \;


    sed -i -e 's/csoundUDPConsole.*//g' Top/argdecode.c

    # Patch 64bit integer clock
    ${patchClock}/bin/patchClock Top/csound.c

    mv include/version.h.in include/version.h
    # see CMakeLists.txt
    substituteInPlace include/version.h \
      --replace "\''${CS_VERSION}" "7" \
      --replace "\''${CS_SUBVER}" "0" \
      --replace "\''${CS_PATCHLEVEL}" "0"

    touch include/float-version.h
    substituteInPlace Top/csmodule.c \
      --replace '#include <dlfcn.h>' ""
    substituteInPlace Engine/csound_orc.y \
      --replace 'csound_orcnerrs' "0"
    substituteInPlace include/sysdep.h \
      --replace '#if defined(HAVE_GCC3) && !defined(SWIG)' \
    '#if defined(HAVE_GCC3) && !defined(WASM_BUILD)'

    # don't open .csound6rc
    substituteInPlace Top/main.c \
      --replace 'checkOptions(csound);' ""

    # follow same preproc defs as emscripten
    # when it come to filesystem calls
    substituteInPlace OOps/diskin2.c \
      --replace '__EMSCRIPTEN__' 'WASM_BUILD'

    substituteInPlace Engine/csound_orc_compile.c \
      --replace '#ifdef EMSCRIPTEN' '#if 1' \
      --replace 'void sanitize(CSOUND *csound) {}' \
                'void sanitize(CSOUND *csound);'

    substituteInPlace Top/one_file.c \
      --replace '#include "corfile.h"' \
            '#include "corfile.h"
             #include <sys/types.h>
             #include <sys/stat.h>
             #include <string.h>
             #include <stdlib.h>
             #include <unistd.h>
             #include <fcntl.h>
             #include <errno.h>' \
             --replace 'umask(0077);' "" \
             --replace 'mkstemp(lbuf)' \
             'open(lbuf, 02)' \
             --replace 'system(sys)' '-1'

    substituteInPlace Engine/linevent.c \
      --replace '#include <ctype.h>' \
         '#include <ctype.h>
          #include <string.h>
          #include <stdlib.h>
          #include <unistd.h>
          #include <fcntl.h>'

    substituteInPlace Opcodes/urandom.c \
      --replace '__HAIKU__' \
        'WASM_BUILD
         #include <unistd.h>'

    substituteInPlace InOut/libmpadec/mp3dec.c \
      --replace '#include "csoundCore.h"' \
                '#include "csoundCore.h"
                 #include <stdlib.h>
                 #include <stdio.h>
                 #include <sys/types.h>
                 #include <unistd.h>
                 '

    substituteInPlace Opcodes/mp3in.c \
      --replace '#include "mp3dec.h"' \
        '#include "mp3dec.h"
         #include <unistd.h>
         #include <fcntl.h>'

    substituteInPlace Top/csound.c \
      --replace 'strcpy(s, "alsa");' 'strcpy(s, "wasi");' \
      --replace 'strcpy(s, "hostbased");' "" \
      --replace 'signal(sigs[i], signal_handler);' "" \
      --replace 'static void psignal' 'static void psignal_' \
      --replace 'HAVE_RDTSC' '__NOT_HERE___' \
      --replace '#if !defined(WIN32)' '#if 0' \
      --replace 'static double timeResolutionSeconds = -1.0;' \
                'static double timeResolutionSeconds = 0.000001;'

    # since we recommend n^2 number,
    # let's make sure that it's default too
    substituteInPlace include/csoundCore.h \
      --replace '#define DFLT_KSMPS 10' \
                '#define DFLT_KSMPS 16' \
      --replace '#define DFLT_KR    FL(4410.0)' \
                '#define DFLT_KR    FL(2756.25)'

    substituteInPlace Top/main.c \
      --replace 'csoundUDPServerStart(csound,csound->oparms->daemon);' "" \
      --replace 'static void put_sorted_score' \
                'extern void put_sorted_score'

    substituteInPlace Engine/musmon.c \
      --replace 'csoundUDPServerClose(csound);' ""

    substituteInPlace Engine/new_orc_parser.c \
      --replace 'csound_orcdebug = O->odebug;' ""

    # date and fs
    sed -i '1s/^/#include <unistd.h>\n/' Opcodes/date.c
    sed -i -e 's/LINUX/1/g' Opcodes/date.c

  '';

  configurePhase = ''
    ${pkgs.bison}/bin/bison -pcsound_orc -d --report=itemset ./Engine/csound_orc.y \
      -o ./Engine/csound_orcparse.c
    # ${pkgs.bison}/bin/bison -pcsound_sco -d --report=itemset ./Engine/csound_sco.y \
    #   --defines=./H/csound_scoparse.h \
    #   -o ./Engine/csound_scoparse.c

    ${pkgs.flex}/bin/flex -B ./Engine/csound_orc.lex > ./Engine/csound_orclex.c
    ${pkgs.flex}/bin/flex -B ./Engine/csound_pre.lex > ./Engine/csound_prelex.c
    ${pkgs.flex}/bin/flex -B ./Engine/csound_prs.lex > ./Engine/csound_prslex.c
    # ${pkgs.flex}/bin/flex -B ./Engine/csound_sco.lex > ./Engine/csound_scolex.c
  '';

  # ../Opcodes/scansyn.c \
  #   ../Opcodes/scansynx.c \
    # expose scansyn_init_ via extern
    # also hardcode away graph console logs
    # as they seem to freeze the browser environment
    # substituteInPlace Opcodes/scansyn.c \
    #   --replace 'static int32_t scansyn_init_' \
    #             'extern int32_t scansyn_init_' \
    #   --replace '*p->i_disp' '0'
    # substituteInPlace Opcodes/scansyn.h \
    #   --replace 'extern int32_t' \
    #             'extern int32_t scansyn_init_(CSOUND *);
    #              extern int32_t'


  buildPhase = ''
    mkdir -p build && cd build
    cp ${./csound_wasm.c} ./csound_wasm.c
    cp ${./unsupported_opcodes.c} ./unsupported_opcodes.c
    cp ${./staticfix.c} ./staticfix.c

    # Why the wasm32-unknown-emscripten triplet:
    # https://bugs.llvm.org/show_bug.cgi?id=42714
    echo "Compile libcsound.wasm"

    ${wasi-sdk}/bin/clang \
      ${if (static == false) then "--target=wasm32-unknown-emscripten" else "--target=wasm32-wasi" } \
      -fno-force-enable-int128 -femulated-tls -fno-exceptions -fno-rtti -Oz \
      --sysroot=${wasi-sdk}/share/wasi-sysroot \
      ${lib.optionalString (static == false) "-fPIC" } \
      -I../H -I../Engine -I../include -I../ \
      -I../InOut/libmpadec -I../Opcodes/emugens \
      -I${libsndfile}/include \
      -I${wasi-sdk}/share/wasi-sysroot/include \
      -I${wasi-sdk}/share/wasi-sysroot/include/c++/v1 \
      -DINIT_STATIC_MODULES=1 \
      -U__MACH__ \
      -UMAC \
      -UMACOSX \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -D_WASI_EMULATED_PROCESS_CLOCKS \
      -D__BUILDING_LIBCSOUND \
      -DWASM_BUILD=1 ${preprocFlags} -c \
      ${lib.optionalString (static == true) "./staticfix.c" } \
      unsupported_opcodes.c \
      ../Engine/auxfd.c \
      ../Engine/cfgvar.c \
      ../Engine/corfiles.c \
      ../Engine/csound_data_structures.c \
      ../Engine/csound_orclex.c \
      ../Engine/csound_orc_compile.c \
      ../Engine/csound_orc_expressions.c \
      ../Engine/csound_orc_optimize.c \
      ../Engine/csound_orc_semantics.c \
      ../Engine/csound_orcparse.c \
      ../Engine/csound_prelex.c \
      ../Engine/csound_prslex.c \
      ../Engine/csound_standard_types.c \
      ../Engine/csound_type_system.c \
      ../Engine/entry1.c \
      ../Engine/envvar.c \
      ../Engine/extract.c \
      ../Engine/fgens.c \
      ../Engine/insert.c \
      ../Engine/srconvert.c \
      ../Engine/udo.c \
      ../Engine/linevent.c \
      ../Engine/memalloc.c \
      ../Engine/memfiles.c \
      ../Engine/musmon.c \
      ../Engine/namedins.c \
      ../Engine/new_orc_parser.c \
      ../Engine/pools.c \
      ../Engine/rdscor.c \
      ../Engine/scope.c \
      ../Engine/scsort.c \
      ../Engine/scxtract.c \
      ../Engine/sort.c \
      ../Engine/sread.c \
      ../Engine/swritestr.c \
      ../Engine/symbtab.c \
      ../Engine/symbtab.c \
      ../Engine/twarp.c \
      ../InOut/circularbuffer.c \
      ../InOut/libmpadec/layer1.c \
      ../InOut/libmpadec/layer2.c \
      ../InOut/libmpadec/layer3.c \
      ../InOut/libmpadec/mp3dec.c \
      ../InOut/libmpadec/mpadec.c \
      ../InOut/libmpadec/synth.c \
      ../InOut/libmpadec/tables.c \
      ../InOut/libsnd.c \
      ../InOut/libsnd_u.c \
      ../InOut/midifile.c \
      ../InOut/midirecv.c \
      ../InOut/midisend.c \
      ../InOut/soundfile.c \
      ../InOut/winEPS.c \
      ../InOut/winascii.c \
      ../InOut/windin.c \
      ../InOut/window.c \
      ../OOps/aops.c \
      ../OOps/bus.c \
      ../OOps/cmath.c \
      ../OOps/compile_ops.c \
      ../OOps/diskin2.c \
      ../OOps/disprep.c \
      ../OOps/dumpf.c \
      ../OOps/fftlib.c \
      ../OOps/goto_ops.c \
      ../OOps/lpred.c \
      ../OOps/midiinterop.c \
      ../OOps/midiops.c \
      ../OOps/midiops2.c \
      ../OOps/midiops3.c \
      ../OOps/midiout.c \
      ../OOps/mxfft.c \
      ../OOps/oscils.c \
      ../OOps/pffft.c \
      ../OOps/pstream.c \
      ../OOps/pitch0.c \
      ../OOps/pvfileio.c \
      ../OOps/pvsanal.c \
      ../OOps/random.c \
      ../OOps/remote.c \
      ../OOps/schedule.c \
      ../OOps/sndinfUG.c \
      ../OOps/str_ops.c \
      ../OOps/ugens1.c \
      ../OOps/ugens2.c \
      ../OOps/ugens3.c \
      ../OOps/ugens4.c \
      ../OOps/ugens5.c \
      ../OOps/ugens6.c \
      ../OOps/ugrw1.c \
      ../OOps/ugtabs.c \
      ../OOps/vdelay.c \
      ../Opcodes/Vosim.c \
      ../Opcodes/afilters.c \
      ../Opcodes/ambicode.c \
      ../Opcodes/ambicode1.c \
      ../Opcodes/arrays.c \
      ../Opcodes/babo.c \
      ../Opcodes/bbcut.c \
      ../Opcodes/bilbar.c \
      ../Opcodes/biquad.c \
      ../Opcodes/bowedbar.c \
      ../Opcodes/buchla.c \
      ../Opcodes/butter.c \
      ../Opcodes/cellular.c \
      ../Opcodes/clfilt.c \
      ../Opcodes/compress.c \
      ../Opcodes/cpumeter.c \
      ../Opcodes/cross2.c \
      ../Opcodes/crossfm.c \
      ../Opcodes/dam.c \
      ../Opcodes/date.c \
      ../Opcodes/dcblockr.c \
      ../Opcodes/dsputil.c \
      ../Opcodes/emugens/emugens.c \
      ../Opcodes/emugens/scugens.c \
      ../Opcodes/eqfil.c \
      ../Opcodes/exciter.c \
      ../Opcodes/fareygen.c \
      ../Opcodes/fareyseq.c \
      ../Opcodes/filter.c \
      ../Opcodes/flanger.c \
      ../Opcodes/fm4op.c \
      ../Opcodes/follow.c \
      ../Opcodes/fout.c \
      ../Opcodes/framebuffer.c \
      ../Opcodes/freeverb.c \
      ../Opcodes/ftconv.c \
      ../Opcodes/ftest.c \
      ../Opcodes/ftgen.c \
      ../Opcodes/gab/gab.c \
      ../Opcodes/gab/hvs.c \
      ../Opcodes/gab/newgabopc.c \
      ../Opcodes/gab/tabmorph.c \
      ../Opcodes/gab/vectorial.c \
      ../Opcodes/gammatone.c \
      ../Opcodes/gendy.c \
      ../Opcodes/grain.c \
      ../Opcodes/grain4.c \
      ../Opcodes/harmon.c \
      ../Opcodes/hrtfearly.c \
      ../Opcodes/hrtferX.c \
      ../Opcodes/hrtfopcodes.c \
      ../Opcodes/hrtfreverb.c \
      ../Opcodes/ifd.c \
      ../Opcodes/liveconv.c \
      ../Opcodes/locsig.c \
      ../Opcodes/loscilx.c \
      ../Opcodes/lowpassr.c \
      ../Opcodes/lufs.c \
      ../Opcodes/mandolin.c \
      ../Opcodes/metro.c \
      ../Opcodes/minmax.c \
      ../Opcodes/modal4.c \
      ../Opcodes/modmatrix.c \
      ../Opcodes/moog1.c \
      ../Opcodes/mp3in.c \
      ../Opcodes/newfils.c \
      ../Opcodes/nlfilt.c \
      ../Opcodes/oscbnk.c \
      ../Opcodes/pan2.c \
      ../Opcodes/partials.c \
      ../Opcodes/partikkel.c \
      ../Opcodes/paulstretch.c \
      ../Opcodes/phisem.c \
      ../Opcodes/physmod.c \
      ../Opcodes/physutil.c \
      ../Opcodes/pinker.c \
      ../Opcodes/pitch.c \
      ../Opcodes/pitchtrack.c \
      ../Opcodes/platerev.c \
      ../Opcodes/pluck.c \
      ../Opcodes/psynth.c \
      ../Opcodes/pvadd.c \
      ../Opcodes/pvinterp.c \
      ../Opcodes/pvlock.c \
      ../Opcodes/pvoc.c \
      ../Opcodes/pvocext.c \
      ../Opcodes/pvread.c \
      ../Opcodes/pvs_ops.c \
      ../Opcodes/pvsband.c \
      ../Opcodes/pvsbasic.c \
      ../Opcodes/pvsbuffer.c \
      ../Opcodes/pvscent.c \
      ../Opcodes/pvsdemix.c \
      ../Opcodes/pvsgendy.c \
      ../Opcodes/quadbezier.c \
      ../Opcodes/repluck.c \
      ../Opcodes/reverbsc.c \
      ../Opcodes/scansyn.c \
      ../Opcodes/scansynx.c \
      ../Opcodes/scoreline.c \
      ../Opcodes/select.c \
      ../Opcodes/seqtime.c \
      ../Opcodes/sequencer.c \
      ../Opcodes/sfont.c \
      ../Opcodes/shaker.c \
      ../Opcodes/shape.c \
      ../Opcodes/singwave.c \
      ../Opcodes/sndloop.c \
      ../Opcodes/sndwarp.c \
      ../Opcodes/space.c \
      ../Opcodes/spat3d.c \
      ../Opcodes/spectra.c \
      ../Opcodes/squinewave.c \
      ../Opcodes/stdopcod.c \
      ../Opcodes/sterrain.c \
      ../Opcodes/syncgrain.c \
      ../Opcodes/tabaudio.c \
      ../Opcodes/tabsum.c \
      ../Opcodes/sc_noise.c \
      ../Opcodes/ugakbari.c \
      ../Opcodes/ugens7.c \
      ../Opcodes/ugens8.c \
      ../Opcodes/ugens9.c \
      ../Opcodes/ugensa.c \
      ../Opcodes/uggab.c \
      ../Opcodes/ugmoss.c \
      ../Opcodes/ugnorman.c \
      ../Opcodes/ugsc.c \
      ../Opcodes/urandom.c \
      ../Opcodes/vaops.c \
      ../Opcodes/vbap.c \
      ../Opcodes/vpvoc.c \
      ../Opcodes/wave-terrain.c \
      ../Opcodes/wterrain2.c \
      ../Opcodes/wpfilters.c \
      ../Opcodes/zak.c \
      ../Top/argdecode.c \
      ../Top/cscore_internal.c \
      ../Top/cscorfns.c \
      ../Top/csdebug.c \
      ../Top/csmodule.c \
      ../Top/getstring.c \
      ../Top/init_static_modules.c \
      ../Top/main.c \
      ../Top/new_opts.c \
      ../Top/one_file.c \
      ../Top/opcode.c \
      ../Top/threads.c \
      ../Top/threadsafe.c \
      ../Top/utility.c \
      ../Top/csound.c \
      ../Opcodes/ampmidid.cpp \
      ../Opcodes/doppler.cpp \
      ../Opcodes/tl/fractalnoise.cpp \
      ../Opcodes/mixer.cpp \
      ../Opcodes/signalflowgraph.cpp \
      ../Opcodes/pvsops.cpp \
      ../Opcodes/bformdec2.cpp \
      ../Opcodes/padsynth_gen.cpp \
      ../Opcodes/arrayops.cpp \
      ../Opcodes/lfsr.cpp \
      ../Opcodes/trigEnvSegs.cpp \
      ../Opcodes/tl/fractalnoise.cpp \
      csound_wasm.c

    #TODO fix ../Opcodes/ftsamplebank.cpp (why does it import thread-local?)
    ${if (static == true) then static-link else dyn-link}

    ${wasi-sdk}/bin/llvm-ar -x ${libsndfile}/lib/libsndfile.a
    ${wasi-sdk}/bin/llvm-ar rcs libcsound-wasm.a ./*.o
  '';

  installPhase = ''
    mkdir -p $out/lib
    mkdir -p $out/include
    cp ./*.wasm $out/lib
    cp ./*.a $out/lib
    cp -rf ../H $out/include
    cp -rf ../Engine $out/include
    cp -rf ../include/* $out/include

    # # make a compressed version for the browser bundle
    ${pkgs.zopfli}/bin/zopfli --zlib -c \
      $out/lib/csound${if (static == true) then ".static" else ".dylib" }.wasm \
        > $out/lib/csound${if (static == true) then ".static" else ".dylib" }.wasm.z
  '';
}
