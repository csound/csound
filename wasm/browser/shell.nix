# A shell environment for a CI test job
let
  pkgs = import <nixpkgs> {};
  google_chrome_ci = pkgs.google-chrome.override { commandLineArgs = "--no-sandbox --disable-dev-shm-usage --headless";};

in pkgs.mkShell {
  CHROME_BIN = "${google_chrome_ci}/bin/google-chrome-stable";
  buildInputs = with pkgs; [
    chromedriver
    google_chrome_ci
    nodejs
    selenium-server-standalone
    vim
    yarn
  ];
}
