{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    git
    pkg-config
    gcc
    qt6.wrapQtAppsHook
    qt6.qttools
  ];

  buildInputs = with pkgs; [
    qt6.qtbase
  ];

  shellHook = ''
    echo "Entering LogLite dev shell"
    echo "Qt6: ${pkgs.qt6.qtbase.version}"
    echo "CMake: $(cmake --version | head -1)"
    echo "Build dir: build/"
  '';
}
