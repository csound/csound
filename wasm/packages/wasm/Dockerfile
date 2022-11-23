FROM nixos/nix

RUN nix-channel --add https://nixos.org/channels/nixpkgs-unstable nixpkgs
RUN nix-channel --update

copy ./src ./src

RUN nix-build -E '(with import <nixpkgs> {}; import ./src/csound.nix)' -o result
