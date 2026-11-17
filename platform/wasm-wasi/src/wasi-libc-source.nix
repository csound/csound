{ fetchFromGitHub }:
{
  version = "0-unstable-2026-11-16";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "f762ab41b0953a4e7340b409697e6625993d60f1";
    sha256 = "0yqw6dnqkcp5b160v75ybb6mhrj4h1rlrg9xagxx009pc3jqfi58";
  };
}
