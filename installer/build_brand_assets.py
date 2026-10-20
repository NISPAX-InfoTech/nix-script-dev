#!/usr/bin/env python3
"""Generate branded WiX bitmaps and product icon from the official NixScript wolf mark."""
from pathlib import Path
import os, shutil
from PIL import Image, ImageDraw, ImageFont, ImageOps
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "installer" / "brand-output"
LOGO = ROOT / "branding" / "nixscript-wolf-blue.webp"
LICENSE = ROOT / "installer" / "preview-license.rtf"

def font(size, bold=False):
    root = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"
    for name in (["segoeuib.ttf", "arialbd.ttf"] if bold else ["segoeui.ttf", "arial.ttf"]):
        try: return ImageFont.truetype(str(root / name), size=size)
        except OSError: pass
    return ImageFont.load_default()

def gradient(width, height, left=(5,58,131), right=(3,113,205)):
    im=Image.new("RGB",(width,height)); pix=im.load()
    for x in range(width):
        t=x/max(1,width-1); c=tuple(round(left[i]*(1-t)+right[i]*t) for i in range(3))
        for y in range(height): pix[x,y]=c
    return im

def main():
    if not LOGO.exists(): raise FileNotFoundError(f"Expected official logo at {LOGO}")
    OUT.mkdir(parents=True, exist_ok=True); shutil.copy2(LICENSE, OUT/"preview-license.rtf")
    logo=Image.open(LOGO).convert("RGB")
    logo.save(OUT/"nixscript.ico", format="ICO", sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
    banner=gradient(493,58); d=ImageDraw.Draw(banner)
    d.polygon([(390,0),(493,0),(493,58),(445,58)],fill=(4,65,145))
    banner.paste(ImageOps.fit(logo,(48,48),method=Image.Resampling.LANCZOS),(5,5))
    d.text((64,11),"NixScript",font=font(25,True),fill=(255,255,255))
    d.text((243,21),"CODE CLEARLY. THINK BIGGER.",font=font(10,True),fill=(184,236,255))
    d.line((64,46,470,46),fill=(83,204,255),width=2); banner.save(OUT/"nixscript-banner.bmp",format="BMP")
    dialog=gradient(493,312,(4,45,106),(6,98,178)); d=ImageDraw.Draw(dialog)
    d.polygon([(0,0),(160,0),(160,312),(0,312)],fill=(4,66,145))
    d.polygon([(0,0),(160,0),(0,160)],fill=(9,116,210)); d.polygon([(0,312),(160,210),(160,312)],fill=(3,54,120))
    dialog.paste(ImageOps.fit(logo,(132,132),method=Image.Resampling.LANCZOS),(14,36))
    d.text((17,180),"NixScript",font=font(20,True),fill=(255,255,255))
    d.text((17,210),"Code clearly.",font=font(12),fill=(194,239,255)); d.text((17,228),"Think bigger.",font=font(12),fill=(194,239,255))
    d.line((188,26,465,26),fill=(165,214,247),width=1); d.polygon([(400,312),(493,219),(493,312)],fill=(227,243,251))
    dialog.save(OUT/"nixscript-dialog.bmp",format="BMP"); print(f"Generated installer art in {OUT}")
if __name__ == "__main__": main()
