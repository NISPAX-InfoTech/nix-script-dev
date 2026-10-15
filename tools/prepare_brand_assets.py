#!/usr/bin/env python3
"""Prepare official NixScript artwork for Android, Windows and Linux."""
from pathlib import Path
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BLUE = ROOT / "branding" / "nixscript-wolf-blue.webp"
DARK = ROOT / "branding" / "nixscript-wolf-dark.webp"

def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)

def main() -> None:
    for source in (BLUE, DARK):
        if not source.is_file():
            raise SystemExit(f"Missing official artwork: {source}")

    # Preserve the optimized official images byte-for-byte in the Android web assets.
    android_assets = ROOT / "android-ide" / "app" / "src" / "main" / "assets" / "branding"
    ensure_dir(android_assets)
    shutil.copy2(BLUE, android_assets / "nixscript-wolf-blue.webp")
    shutil.copy2(DARK, android_assets / "nixscript-wolf-dark.webp")

    with Image.open(BLUE) as source:
        blue = source.convert("RGBA")
    with Image.open(DARK) as source:
        dark = source.convert("RGBA")

    # Android adaptive icon foreground and density-specific pre-26 launcher icons.
    res = ROOT / "android-ide" / "app" / "src" / "main" / "res"
    nodpi = res / "drawable-nodpi"
    ensure_dir(nodpi)
    blue.resize((512, 512), Image.Resampling.LANCZOS).save(nodpi / "nixscript_wolf_blue.png", optimize=True)
    for density, size in (("mdpi", 48), ("hdpi", 72), ("xhdpi", 96), ("xxhdpi", 144), ("xxxhdpi", 192)):
        folder = res / f"mipmap-{density}"
        ensure_dir(folder)
        blue.resize((size, size), Image.Resampling.LANCZOS).save(folder / "ic_launcher.png", optimize=True)

    # Windows Explorer file icon and Linux MIME icon are generated from the blue official artwork.
    win_dir = ROOT / "platform" / "windows"
    linux_dir = ROOT / "platform" / "linux"
    ensure_dir(win_dir)
    ensure_dir(linux_dir)
    blue_icon = blue.resize((256, 256), Image.Resampling.LANCZOS)
    blue_icon.save(win_dir / "nixscript-ns.ico", format="ICO",
                   sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    blue.resize((512, 512), Image.Resampling.LANCZOS).save(linux_dir / "nixscript-ns.png", optimize=True)
    print("Prepared official NixScript wolf assets for Android, Windows and Linux.")

if __name__ == "__main__":
    main()
