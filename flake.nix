{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-24.11";
  };

  outputs = { self, nixpkgs }: 
  let
    forEachSystem = f: nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ]
      (system: f nixpkgs.legacyPackages.${system});
  in {
    overlays = (final: prev: {
      ssplice-cpp = prev.callPackage ./nix/package.nix { };
    });
    packages = forEachSystem (pkgs: {
      ssplice-cpp = pkgs.callPackage ./nix/package.nix { };
    });
    devShells = forEachSystem (pkgs: {
      default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          meson
          pkg-config
          ninja
        ];
        buildInputs = with pkgs; [
          kdePackages.qtbase
          duktape
          curl
          libarchive
          kdePackages.qtwayland
        ];
      };
    });
  };
}
