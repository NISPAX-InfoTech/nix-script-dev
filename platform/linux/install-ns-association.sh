#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "Pillow is required. Install it with: python3 -m pip install --user Pillow" >&2
  exit 1
fi
python3 "$ROOT/tools/prepare_brand_assets.py"
ICON_HOME="$HOME/.local/share/icons/hicolor"
MIME_HOME="$HOME/.local/share/mime"
install -D -m 0644 "$ROOT/platform/linux/nixscript-ns.png" "$ICON_HOME/512x512/mimetypes/text-x-nixscript.png"
install -D -m 0644 "$ROOT/platform/linux/nixscript-mime.xml" "$MIME_HOME/packages/nixscript-mime.xml"
if command -v update-mime-database >/dev/null 2>&1; then update-mime-database "$MIME_HOME"; fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then gtk-update-icon-cache -f "$ICON_HOME" >/dev/null 2>&1 || true; fi
echo "Installed the official NixScript wolf icon for .ns files. Your desktop may need a refresh or logout/login."
