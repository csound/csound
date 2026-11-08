"""Check PipeWire backend selection against isolated dependency installations."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


PROJECT = r'''
cmake_minimum_required(VERSION 3.13.4)
project(PipeWireDetection NONE)
list(PREPEND CMAKE_MODULE_PATH "${CSOUND_SOURCE_DIR}/cmake/Modules")
include("${CSOUND_SOURCE_DIR}/cmake/cmake-utilities.cmake")
set(APPLE FALSE)
set(WIN32 FALSE)
set(LINUX FALSE)
foreach(backend PORTAUDIO PORTMIDI JACK ALSA PULSEAUDIO IPMIDI)
    set(USE_${backend} OFF CACHE BOOL "" FORCE)
endforeach()
set(CMAKE_DISABLE_FIND_PACKAGE_PulseAudio TRUE)
function(make_plugin name)
    add_library(${name} INTERFACE)
endfunction()
add_subdirectory("${CSOUND_SOURCE_DIR}/InOut" inout)
if(EXPECT_PIPEWIRE)
    if(NOT TARGET rtpw)
        message(FATAL_ERROR "Supported PipeWire installation was rejected")
    endif()
elseif(TARGET rtpw)
    message(FATAL_ERROR "Unsupported PipeWire installation enabled rtpw")
endif()
'''


def installation(prefix, version, metadata=True, version_header=True):
    legacy = version.startswith('0.2.')
    api, spa = ('0.2', '0.1') if legacy else ('0.3', '0.2')
    include = prefix / 'include'
    pw_include = include if legacy else include / 'pipewire-0.3'
    spa_include = include if legacy else include / 'spa-0.2'
    (pw_include / 'pipewire').mkdir(parents=True)
    (spa_include / 'spa/param').mkdir(parents=True)
    (pw_include / 'pipewire/pipewire.h').touch()
    if version_header:
        (pw_include / 'pipewire/version.h').write_text(
            '#define pw_get_headers_version() ("' + version + '")\n')
    (spa_include / 'spa/param/props.h').touch()
    (prefix / 'lib/pkgconfig').mkdir(parents=True)
    (prefix / 'lib' / ('libpipewire-' + api + '.so')).touch()
    (prefix / 'lib' / ('libpipewire-' + api + '.dylib')).touch()
    if metadata:
        for name, inc, libs in [
                ('libpipewire-' + api, pw_include, '-lpipewire-' + api),
                ('libspa-' + spa, spa_include, '')]:
            (prefix / 'lib/pkgconfig' / (name + '.pc')).write_text(
                f'Name: {name}\nDescription: test fixture\nVersion: {version}\n'
                f'Libs: -L{prefix / "lib"} {libs}\nCflags: -I{inc}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--pkg-config', default='pkg-config')
    args = parser.parse_args()
    args.pkg_config = shutil.which(args.pkg_config) or args.pkg_config
    source = Path(__file__).resolve().parents[1]
    cases = [
        ('legacy', '0.2.5', False, True, True, False),
        ('early-0.3', '0.3.49', False, True, True, False),
        ('minimum', '0.3.50', True, True, True, False),
        ('current', '1.2.7', True, True, True, False),
        ('no-metadata', '0.3.65', True, False, True, False),
        ('no-pkg-config', '0.3.65', True, False, True, True),
        ('unknown-version', '0.3.65', False, True, False, False),
        ('stale-headers', '0.3.65', False, True, True, False),
    ]
    failures = []
    with tempfile.TemporaryDirectory(prefix='csound-pipewire-') as directory:
        root = Path(directory)
        project = root / 'project'
        project.mkdir()
        (project / 'CMakeLists.txt').write_text(PROJECT)
        for name, version, expected, metadata, header, no_pkg in cases:
            prefix = root / name / 'prefix'
            installation(prefix, version, metadata, header)
            command = [args.cmake, '-S', str(project), '-B', str(root / name / 'build'),
                       '-DCSOUND_SOURCE_DIR=' + str(source),
                       '-DEXPECT_PIPEWIRE=' + ('ON' if expected else 'OFF'),
                       '-DCMAKE_PREFIX_PATH=' + str(prefix),
                       '-DCMAKE_FIND_ROOT_PATH=' + str(prefix),
                       '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY',
                       '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY',
                       '-DPKG_CONFIG_EXECUTABLE=' + args.pkg_config]
            if no_pkg:
                command.append('-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE')
            if name == 'stale-headers':
                old = root / name / 'old'
                installation(old, '0.2.5')
                command += ['-DPipeWire_INCLUDE_DIRS=' + str(old / 'include'),
                            '-DSpa_INCLUDE_DIRS=' + str(old / 'include')]
            env = dict(os.environ, PKG_CONFIG_PATH='', PKG_CONFIG_SYSROOT_DIR='',
                       PKG_CONFIG_LIBDIR=str(prefix / 'lib/pkgconfig'))
            result = subprocess.run(command, env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            print(f'{name}: {"PASS" if result.returncode == 0 else "FAIL"}')
            if result.returncode:
                failures.append(name)
                print(result.stdout)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
