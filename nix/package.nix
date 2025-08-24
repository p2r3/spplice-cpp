{
  lib,
  stdenv,
  meson,
  kdePackages,
  duktape,
  curl,
  libarchive,
  pkg-config,
  ninja,
}:

stdenv.mkDerivation {
  pname = "splice-cpp";
  version = "0.9.5";

  src = lib.cleanSource ../.;

  nativeBuildInputs = [
    meson
    kdePackages.wrapQtAppsHook
    pkg-config
    ninja
  ];

  buildInputs = [
    kdePackages.qtbase
    duktape
    curl
    libarchive
  ];

  mesonFlags = [
    (lib.mesonBool "USE_QT6" true)
  ];
}
