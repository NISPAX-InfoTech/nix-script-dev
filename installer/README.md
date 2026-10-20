# NixScript installation packages

Desktop releases are distributed as native package installers, not ZIP archives: Windows MSI, Linux DEB/RPM, and macOS PKG. The VS Code extension is a VSIX; Android is an APK.

## Windows MSI behavior

The MSI installs nix.exe into %ProgramFiles%\NixScript and adds that directory to the machine PATH. It also sets NIXSCRIPT_HOME, NIXSCRIPT_LIB, NIXSCRIPT_INCLUDE, and NIXSCRIPT_PACKAGES as system environment variables. It provisions %ProgramData%\NixScript\lib, include, and packages with write permissions for the built-in Windows Users group. Open a new terminal after install.

The uninstaller removes the CLI and PATH/environment registrations but preserves non-empty shared library/package directories.

## Compiler note

The interpreter is a prebuilt C++ executable; end users do not need g++ to run .ns programs. g++ is not silently installed by the MSI. A compiler is needed only for workflows that explicitly build native C/C++ extensions. The library/package directories are separate from Program Files so NixScript package files do not need to be placed beside the executable.

## Linux and macOS paths

Linux installs nix at /usr/bin/nix, documentation under /usr/share/doc/nixscript, global package data under /var/lib/nixscript, and per-user paths under ~/.nixscript. macOS installs nix at /usr/local/bin/nix and documentation under /usr/local/share/nixscript; /etc/paths.d/nixscript registers /usr/local/bin.
