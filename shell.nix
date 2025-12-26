# A standalone shell definition that downloads and uses packages from `nixpkgs-esp-dev` automatically.

let
  nixpkgs-esp-dev = builtins.fetchGit {
    url = "https://github.com/mirrexagon/nixpkgs-esp-dev.git";

    # Optionally pin to a specific commit of `nixpkgs-esp-dev`.
    # rev = "<commit hash>";
  };

  pkgs = import <nixpkgs> { 
    overlays = [ (import "${nixpkgs-esp-dev}/overlay.nix") ]; 
    config = {
      permittedInsecurePackages = [
        "python3.12-ecdsa-0.19.1"
      ];
    };
  };
in
pkgs.mkShell {
  name = "IDF";

  buildInputs = with pkgs; [
    esp-idf-full
  ];
  shellHook = ''
    unset SOURCE_DATE_EPOCH
  '';
}
