{ fetchFromGitHub }:
{
  version = "0-unstable-2026-11-05";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "ce2ee9fb7e39c20ae266d294dd2d0afd1bf1f8e2";
    sha256 = "1cv9i6p63wqfa7dfg0h6zggala1alyi4kl0lxblg9gqjwx3b04x2";
  };
}
