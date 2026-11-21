{ fetchFromGitHub }:
{
  version = "0-unstable-2026-11-18";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "c3b3596e3d3b2418cb128c0095ad710d351959d6";
    sha256 = "1za2hsk22m804flhs73i8fzxw4k7ph578a16sk53y0nx92nc4wdi";
  };
}
