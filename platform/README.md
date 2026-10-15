# NixScript file-type icons

The Android launcher uses a blue-and-white geometric wolf emblem inspired by the NixScript artwork.

## Windows

1. Place a square wolf icon at `%LOCALAPPDATA%\\NixScript\\nixscript-ns.ico`.
2. Double-click `platform/windows/associate-ns.reg` and approve the registry change.
3. `.ns` files will use the NixScript source-file association and that icon. The sample open action uses Notepad; change it to the installed NixScript IDE executable when one is available.

Windows Explorer expects an ICO file. Convert the selected square wolf artwork to ICO at 256px or larger and place it at the path above.

## Linux

Install `platform/linux/nixscript-mime.xml` to `~/.local/share/mime/packages/nixscript-mime.xml`, and `platform/linux/nixscript-ns.desktop` to `~/.local/share/applications/nixscript-ns.desktop`. Install a square wolf PNG named `nixscript-ns.png` under `~/.local/share/icons/hicolor/512x512/apps/`.

Then run `update-mime-database ~/.local/share/mime` and `update-desktop-database ~/.local/share/applications` (if available). Adjust the desktop entry to launch the installed NixScript IDE.

Note: the Android vector is a scalable approximation of the supplied artwork. The original raster files are not yet embedded in this repository, and Windows ICO/Linux PNG files must be generated from the final artwork before those desktop associations display the matching icon.
