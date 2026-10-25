# PFFFT

Unmodified sources from [marton78/pffft at aa16fd3db58de4ba5dae8b0438440bb9da46b6fa](https://github.com/marton78/pffft/tree/aa16fd3db58de4ba5dae8b0438440bb9da46b6fa).
The licence is in [LICENSE.txt](LICENSE.txt).

This commit is newer than release v1.1.0. It includes the native double NEON
and WebAssembly SIMD implementations and the Windows ARM64 detection fix.
Only the FFT sources, headers and SIMD helpers are included.

Csound builds these through the wrappers in OOps. The wrappers select SIMD
only when the target enables it and honour PFFFT_SIMD_DISABLE. They do not
add AVX or WebAssembly SIMD requirements to a build.

Csound's private adapter in H/pffft-csound.h uses the float or double API
to match its sample type. The Windows installers still ship H/pffft.h for
users of the legacy float API; the core no longer includes it. The default
FFT remains Csound's original implementation.
