# Official NixScript wolf branding and .ns icons

The two files in `branding/` are the official NixScript artwork supplied by the project owner:
- `nixscript-wolf-blue.webp`: blue version for the app and file icons.
- `nixscript-wolf-dark.webp`: dark-background version used inside the Android IDE.

They are compact, resized encodings of the supplied images; no replacement or redrawn artwork is used. `tools/prepare_brand_assets.py` derives required platform formats from those files.

## Android

The APK workflow runs the branding preparation script before Gradle. The app header uses the official blue wolf, the About panel uses the official dark wolf, and the launcher icon resources are generated from the blue wolf.

## Windows

From a PowerShell window in the repository, run:
```powershell
powershell -ExecutionPolicy Bypass -File .\platform\windows\install-ns-association.ps1
```
The script generates and installs `nixscript-ns.ico`, and sets the current user's `.ns` file icon. It deliberately does not choose an editor/open command because a native Windows NixScript IDE executable is not currently packaged.

## Linux

Install Pillow for Python 3 if it is not already available, then run:
```sh
sh ./platform/linux/install-ns-association.sh
```
The script installs the shared MIME definition and official wolf PNG icon for `text/x-nixscript` / `*.ns`. Linux desktop environments vary; a MIME/icon cache refresh or logout/login may be needed.

The generated ICO/PNG and Android density variants are build/installer outputs derived from the two official assets; the artwork itself is not redrawn.
