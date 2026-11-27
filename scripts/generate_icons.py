#!/usr/bin/env python3
"""
Libris Unified UI Icon Generator
Generates 40 crisp, optically consistent 96x96 RGBA PNG icons from handcrafted 24x24 SVGs:
- assets/icons/ic_<name>.png        (#FFFFFF pure white for Dark mode & BB10 native chrome)
- assets/icons/ic_<name>_dark.png   (#1A1A1A dark charcoal for Light mode)
- assets/icons/ic_<name>_sepia.png  (#3E2D1D warm sepia ink for Sepia reader overlays)

Usage:
    python3 scripts/generate_icons.py [--preview /tmp/libris_icon_sheet.png]
"""

import argparse
import io
import math
import os
import subprocess
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = "assets/icons"
ICON_SIZE = 96
RENDER_SIZE = 384

COLOR_DARK_MODE = (255, 255, 255)   # #FFFFFF
COLOR_LIGHT_MODE = (26, 26, 26)     # #1A1A1A
COLOR_SEPIA_MODE = (62, 45, 29)     # #3E2D1D

SEPIA_ICONS = {"previous", "next", "clear", "search", "settings", "display_aa", "crop"}


def _star_polygon_path(cx=12.0, cy=12.35, outer_r=8.2, inner_r=3.65):
    pts = []
    for i in range(10):
        angle = -math.pi / 2.0 + i * (math.pi / 5.0)
        r = outer_r if i % 2 == 0 else inner_r
        x = cx + r * math.cos(angle)
        y = cy + r * math.sin(angle)
        pts.append(f"{x:.2f},{y:.2f}")
    return "M" + " L".join(pts) + " Z"


STAR_PATH = _star_polygon_path()

ICONS = {
    "add": '<path d="M12 5v14M5 12h14"/>',
    "book": (
        '<path d="M5 18.5V5.5A2 2 0 0 1 7 3.5h11.5a.5.5 0 0 1 .5.5v13H7a2 2 0 0 0-2 2z"/>'
        '<path d="M5 18.5A2 2 0 0 0 7 20.5h12v-3.5"/>'
        '<path d="M9 3.5v13.5"/>'
    ),
    "bookmark_add": (
        '<path d="M15 10.5V5a1.5 1.5 0 0 0-1.5-1.5H6.5A1.5 1.5 0 0 0 5 5v14.5l5-3.3 2 1.32"/>'
        '<path d="M17.2 13.8v5.4M14.5 16.5h5.4"/>'
    ),
    "bookmark_filled": (
        '<path d="M6.5 5A1.5 1.5 0 0 1 8 3.5h8A1.5 1.5 0 0 1 17.5 5v15.5l-5.5-3.7-5.5 3.7V5z" '
        'fill="#FFFFFF" stroke="#FFFFFF" stroke-width="1.85"/>'
    ),
    "bookmarks": '<path d="M6.5 5A1.5 1.5 0 0 1 8 3.5h8A1.5 1.5 0 0 1 17.5 5v15.5l-5.5-3.7-5.5 3.7V5z"/>',
    "calendar": (
        '<rect x="3.5" y="5" width="17" height="15.5" rx="2"/>'
        '<path d="M8 3.2v3.3M16 3.2v3.3M3.5 9.8h17"/>'
        '<circle cx="7.5" cy="13.3" r="0.95" fill="#FFFFFF" stroke="none"/>'
        '<circle cx="12" cy="13.3" r="0.95" fill="#FFFFFF" stroke="none"/>'
        '<circle cx="16.5" cy="13.3" r="0.95" fill="#FFFFFF" stroke="none"/>'
        '<circle cx="7.5" cy="17.2" r="0.95" fill="#FFFFFF" stroke="none"/>'
        '<path d="M13.6 16.8l1.6 1.6 3.1-3.1"/>'
    ),
    "chart": (
        '<path d="M3.5 19.5h17"/>'
        '<rect x="5" y="12.5" width="3.5" height="7" rx="0.7"/>'
        '<rect x="10.25" y="5" width="3.5" height="14.5" rx="0.7"/>'
        '<rect x="15.5" y="9" width="3.5" height="10.5" rx="0.7"/>'
    ),
    "clear": '<path d="M6 6l12 12M18 6L6 18"/>',
    "crop": '<path d="M7 3.5V16a1.5 1.5 0 0 0 1.5 1.5H20.5M3.5 7H16a1.5 1.5 0 0 1 1.5 1.5V20.5"/>',
    "display_aa": (
        '<path d="M4.5 17.5L8.2 6.5l3.7 11M5.8 14h4.8"/>'
        '<path d="M18.8 11.5v6M18.8 14.5a2.6 2.6 0 1 1-5.2 0 2.6 2.6 0 0 1 5.2 0z"/>'
    ),
    "done": '<path d="M4.5 12.5l5 5 10-10.5"/>',
    "download": '<path d="M12 4v11M7.5 11l4.5 4.5L16.5 11M4.5 16.5V18a2 2 0 0 0 2 2h11a2 2 0 0 0 2-2v-1.5"/>',
    "exit": '<path d="M10.5 4.5H6A1.5 1.5 0 0 0 4.5 6v12A1.5 1.5 0 0 0 6 19.5h4.5M15 8l4.5 4-4.5 4M9.5 12h10"/>',
    "export": (
        '<path d="M12 14.5V4M8 8l4-4 4 4"/>'
        '<path d="M4.5 14.5V18a2 2 0 0 0 2 2h11a2 2 0 0 0 2-2v-3.5"/>'
    ),
    "filter": '<path d="M4 5h16l-6.2 7.3v5.4l-3.6 1.8v-7.2L4 5z"/>',
    "folder": '<path d="M3.5 7A1.5 1.5 0 0 1 5 5.5h4.2l1.8 2H19A1.5 1.5 0 0 1 20.5 9v9A1.5 1.5 0 0 1 19 19.5H5A1.5 1.5 0 0 1 3.5 18V7z"/>',
    "grid": (
        '<rect x="4" y="4" width="6.5" height="6.5" rx="1.4"/>'
        '<rect x="13.5" y="4" width="6.5" height="6.5" rx="1.4"/>'
        '<rect x="4" y="13.5" width="6.5" height="6.5" rx="1.4"/>'
        '<rect x="13.5" y="13.5" width="6.5" height="6.5" rx="1.4"/>'
    ),
    "highlight": (
        '<path d="M16 4l4 4-9 9-4.5.5.5-4.5L16 4z"/>'
        '<path d="M13.5 6.5l4 4"/>'
        '<path d="M4.5 20.2h15"/>'
    ),
    "home": (
        '<path d="M3.8 11.2L12 4.2l8.2 7"/>'
        '<path d="M5.8 9.8V19a1.2 1.2 0 0 0 1.2 1.2h10a1.2 1.2 0 0 0 1.2-1.2V9.8"/>'
        '<path d="M9.8 20.2v-5.4a1 1 0 0 1 1-1h2.4a1 1 0 0 1 1 1v5.4"/>'
    ),
    "import": (
        '<path d="M12 4v10M8.2 10.5L12 14.3l3.8-3.8"/>'
        '<path d="M4.5 14.5V18a2 2 0 0 0 2 2h11a2 2 0 0 0 2-2v-3.5"/>'
    ),
    "info": (
        '<circle cx="12" cy="12" r="8.5"/>'
        '<path d="M12 11.5v4.5"/>'
        '<circle cx="12" cy="8.2" r="1.0" fill="#FFFFFF" stroke="none"/>'
    ),
    "library": (
        '<path d="M3.5 19.5h17"/>'
        '<path d="M5 19.5V7h3.8v12.5M5 10.2h3.8"/>'
        '<path d="M8.8 19.5V5h4v14.5M8.8 8.5h4"/>'
        '<path d="M15 19.5L13 7.2l3.5-.7 2 13"/>'
    ),
    "list": (
        '<circle cx="5.2" cy="7" r="1.15" fill="#FFFFFF" stroke="none"/><path d="M9 7h10.5"/>'
        '<circle cx="5.2" cy="12" r="1.15" fill="#FFFFFF" stroke="none"/><path d="M9 12h10.5"/>'
        '<circle cx="5.2" cy="17" r="1.15" fill="#FFFFFF" stroke="none"/><path d="M9 17h10.5"/>'
    ),
    "mode_dark": (
        '<path d="M19.5 14.2A7.5 7.5 0 0 1 9.8 4.5a7.5 7.5 0 1 0 9.7 9.7z"/>'
        '<path d="M17.5 5v3M16 6.5h3"/>'
    ),
    "mode_light": (
        '<circle cx="12" cy="12" r="4"/>'
        '<path d="M12 3.8v1.6M12 18.6v1.6M3.8 12h1.6M18.6 12h1.6'
        'M6.2 6.2l1.15 1.15M16.65 16.65l1.15 1.15M17.8 6.2l-1.15 1.15M7.35 16.65L6.2 17.8"/>'
    ),
    "mode_sepia": (
        '<path d="M6.5 3.5h7.5l4.5 4.5V19a1.5 1.5 0 0 1-1.5 1.5H6.5A1.5 1.5 0 0 1 5 19V5A1.5 1.5 0 0 1 6.5 3.5z"/>'
        '<path d="M14 3.5V8h4.5"/>'
        '<path d="M8.5 12h7M8.5 15.5h5"/>'
    ),
    "next": '<path d="M9 5.5l6.5 6.5L9 18.5"/>',
    "open": (
        '<path d="M10.5 5H6A1.5 1.5 0 0 0 4.5 6.5v11A1.5 1.5 0 0 0 6 19h11a1.5 1.5 0 0 0 1.5-1.5V13"/>'
        '<path d="M14 5h5v5M11 13l8-8"/>'
    ),
    "open_file": (
        '<path d="M3.5 18.2V6.8A1.5 1.5 0 0 1 5 5.3h4.2l1.8 2h7A1.5 1.5 0 0 1 19.5 8.8v1.7"/>'
        '<path d="M3.5 18.2l2.1-6.7A1.5 1.5 0 0 1 7 10.5h12.8a1.2 1.2 0 0 1 1.15 1.55l-1.9 6.2A1.5 1.5 0 0 1 17.6 19.3H5A1.5 1.5 0 0 1 3.5 18.2z"/>'
    ),
    "opds": (
        '<path d="M7 16.5H6a3.5 3.5 0 0 1-.4-6.98A5.2 5.2 0 0 1 15.8 8a3.8 3.8 0 0 1 2.2 7.2"/>'
        '<path d="M12 11.5v8M9.3 16.8L12 19.5l2.7-2.7"/>'
    ),
    "page": (
        '<rect x="5" y="4.5" width="14" height="15" rx="1.5"/>'
        '<path d="M10.2 10L8 12l2.2 2M13.8 10L16 12l-2.2 2M8.5 12h7"/>'
    ),
    "previous": '<path d="M15 5.5L8.5 12 15 18.5"/>',
    "reading": (
        '<path d="M12 6.5c-1.8-1.5-4.5-2-7.5-1.8v12.8c3-.2 5.7.3 7.5 1.8'
        'm0-12.8c1.8-1.5 4.5-2 7.5-1.8v12.8c-3-.2-5.7.3-7.5 1.8m0-12.8v12.8"/>'
    ),
    "recent": (
        '<path d="M5.5 8.25A7.5 7.5 0 1 1 5.5 15.75"/>'
        '<path d="M5 5.2v3.3h3.3"/>'
        '<path d="M12 8v4l2.8 1.8"/>'
    ),
    "refresh": (
        '<path d="M18.5 15.75A7.5 7.5 0 1 1 18.5 8.25"/>'
        '<path d="M19 5.2v3.3h-3.3"/>'
    ),
    "scroll": (
        '<rect x="5" y="7" width="14" height="10" rx="1.5"/>'
        '<path d="M8.5 10.5h7M8.5 13.5h5"/>'
        '<path d="M9.5 5L12 3.2 14.5 5M9.5 19L12 20.8 14.5 19"/>'
    ),
    "sdcard": (
        '<path d="M6.5 4h7.8l3.2 3.2V19a1.5 1.5 0 0 1-1.5 1.5H6.5A1.5 1.5 0 0 1 5 19V5.5A1.5 1.5 0 0 1 6.5 4z"/>'
        '<path d="M9 7.2v2.6M11.5 7.2v2.6M14 7.2v2.6"/>'
    ),
    "search": '<circle cx="10.5" cy="10.5" r="6.2"/><path d="M15.2 15.2l4.5 4.5"/>',
    "settings": (
        '<circle cx="12" cy="12" r="3.2"/>'
        '<circle cx="12" cy="12" r="6.4"/>'
        '<path d="M12 3.3v2.3M12 18.4v2.3M3.3 12h2.3M18.4 12h2.3'
        'M5.85 5.85l1.62 1.62M16.53 16.53l1.62 1.62M18.15 5.85l-1.62 1.62M7.47 16.53l-1.62 1.62"/>'
    ),
    "shelf_manage": (
        '<path d="M3.5 19.5h8.5"/>'
        '<path d="M5 19.5V6.5h3.2v13M5 9.8h3.2"/>'
        '<path d="M8.2 19.5V4.5h3.5v15M8.2 8.2h3.5"/>'
        '<path d="M17 11.5v7M13.5 15h7"/>'
    ),
    "star": f'<path d="{STAR_PATH}"/>',
    "star_filled": f'<path d="{STAR_PATH}" fill="#FFFFFF" stroke="#FFFFFF" stroke-width="1.85"/>',
    "storage": (
        '<rect x="6.5" y="3.5" width="11" height="17" rx="2"/>'
        '<path d="M10.5 6.2h3"/>'
        '<circle cx="12" cy="17.5" r="0.95" fill="#FFFFFF" stroke="none"/>'
    ),
    "text_mode": (
        '<path d="M4.5 15.8L8 6.5l3.5 9.3M5.7 13h4.6"/>'
        '<path d="M14 8h5.5M14 12h5.5M4.5 19h15"/>'
    ),
    "theme": (
        '<circle cx="12" cy="12" r="8.5"/>'
        '<path d="M12 3.5a8.5 8.5 0 0 1 0 17V3.5z" fill="#FFFFFF" stroke="none"/>'
    ),
    "toc": (
        '<path d="M4.5 6.5h15"/>'
        '<circle cx="6.2" cy="12" r="1.1" fill="#FFFFFF" stroke="none"/><path d="M9.5 12h10"/>'
        '<circle cx="6.2" cy="17.5" r="1.1" fill="#FFFFFF" stroke="none"/><path d="M9.5 17.5h8.5"/>'
    ),
    "translate": (
        '<path d="M4.5 7h8M8.5 4.8V7M11 7c-.5 3.3-2.5 5.8-5.5 7.2M6.5 9.5c1.2 1.8 2.8 3.2 4.8 4.2"/>'
        '<path d="M12.5 19.2L16 10.5l3.5 8.7M13.8 16.5h4.4"/>'
    ),
    "tts": (
        '<path d="M4.5 9.5h3.2l4.5-3.8v12.6l-4.5-3.8H4.5a1 1 0 0 1-1-1v-3a1 1 0 0 1 1-1z"/>'
        '<path d="M15.5 9.2a4 4 0 0 1 0 5.6"/>'
        '<path d="M18 6.8a7.4 7.4 0 0 1 0 10.4"/>'
    ),
    "wifi": (
        '<circle cx="12" cy="18" r="1.2" fill="#FFFFFF" stroke="none"/>'
        '<path d="M9.03 15.03a4.2 4.2 0 0 1 5.94 0"/>'
        '<path d="M6.34 12.34a8 8 0 0 1 11.32 0"/>'
        '<path d="M3.66 9.66a11.8 11.8 0 0 1 16.68 0"/>'
    ),
}


def build_svg(body):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" '
        f'width="{RENDER_SIZE}" height="{RENDER_SIZE}" '
        f'fill="none" stroke="#FFFFFF" stroke-width="1.85" '
        f'stroke-linecap="round" stroke-linejoin="round">\n  {body}\n</svg>'
    )


def render_alpha_mask(svg_text):
    proc = subprocess.run(
        ["magick", "-background", "none", "svg:-", "-resize", f"{ICON_SIZE}x{ICON_SIZE}", "png:-"],
        input=svg_text.encode("utf-8"),
        capture_output=True,
        check=True,
    )
    img = Image.open(io.BytesIO(proc.stdout)).convert("RGBA")
    return img.getchannel("A")


def colorize_from_alpha(alpha_band, rgb):
    r_val, g_val, b_val = rgb
    w, h = alpha_band.size
    r = Image.new("L", (w, h), r_val)
    g = Image.new("L", (w, h), g_val)
    b = Image.new("L", (w, h), b_val)
    return Image.merge("RGBA", (r, g, b, alpha_band))


def generate_all():
    os.makedirs(OUT_DIR, exist_ok=True)
    generated = []
    for name in sorted(ICONS.keys()):
        alpha = render_alpha_mask(build_svg(ICONS[name]))

        white_img = colorize_from_alpha(alpha, COLOR_DARK_MODE)
        white_path = os.path.join(OUT_DIR, f"ic_{name}.png")
        white_img.save(white_path, "PNG", optimize=True)
        generated.append(white_path)

        dark_img = colorize_from_alpha(alpha, COLOR_LIGHT_MODE)
        dark_path = os.path.join(OUT_DIR, f"ic_{name}_dark.png")
        dark_img.save(dark_path, "PNG", optimize=True)
        generated.append(dark_path)

        if name in SEPIA_ICONS:
            sepia_img = colorize_from_alpha(alpha, COLOR_SEPIA_MODE)
            sepia_path = os.path.join(OUT_DIR, f"ic_{name}_sepia.png")
            sepia_img.save(sepia_path, "PNG", optimize=True)
            generated.append(sepia_path)
    return generated


def build_contact_sheet(preview_path):
    names = sorted(ICONS.keys())
    cols = 8
    rows = math.ceil(len(names) / cols)
    cell_w = 136
    cell_h = 112
    pad = 24
    header_h = 38

    dark_h = header_h + rows * cell_h
    light_h = header_h + rows * cell_h
    sepia_h = header_h + 112

    sheet_w = cols * cell_w + pad * 2
    sheet_h = pad + dark_h + 16 + light_h + 16 + sepia_h + pad

    sheet = Image.new("RGB", (sheet_w, sheet_h), (24, 24, 27))
    draw = ImageDraw.Draw(sheet)

    font_path = "/usr/share/fonts/open-sans/OpenSans-Regular.ttf"
    if not os.path.exists(font_path):
        font_path = "/usr/share/fonts/google-noto/NotoSans-Regular.ttf"
    try:
        font = ImageFont.truetype(font_path, 13)
        font_hdr = ImageFont.truetype(font_path, 16)
    except Exception:
        font = ImageFont.load_default()
        font_hdr = font

    def draw_section(title, y0, bg_rgb, txt_rgb, item_names, suffix):
        sec_rows = math.ceil(len(item_names) / cols)
        sec_h = header_h + sec_rows * cell_h
        draw.rectangle([pad, y0, sheet_w - pad, y0 + sec_h], fill=bg_rgb, outline=(70, 70, 75))
        draw.text((pad + 14, y0 + 10), title, font=font_hdr, fill=txt_rgb)
        for idx, name in enumerate(item_names):
            r = idx // cols
            c = idx % cols
            cx = pad + c * cell_w + cell_w // 2
            cy = y0 + header_h + r * cell_h + 44
            icon_path = os.path.join(OUT_DIR, f"ic_{name}{suffix}.png")
            ic = Image.open(icon_path).convert("RGBA").resize((64, 64), Image.Resampling.LANCZOS)
            sheet.paste(ic, (cx - 32, cy - 32), ic)
            draw.text((cx, cy + 44), name, font=font, fill=txt_rgb, anchor="mm")
        return y0 + sec_h

    y = pad
    y = draw_section("DARK MODE / BB10 CHROME (#FFFFFF on #111111)", y, (17, 17, 17), (240, 240, 240), names, "")
    y += 16
    y = draw_section("LIGHT MODE (#1A1A1A on #FFFFFF)", y, (255, 255, 255), (26, 26, 26), names, "_dark")
    y += 16
    draw_section("READER SEPIA MODE (#3E2D1D on #F4ECD8)", y, (244, 236, 216), (62, 45, 29), sorted(SEPIA_ICONS), "_sepia")

    sheet.save(preview_path, "PNG")


def main():
    parser = argparse.ArgumentParser(description="Generate Libris unified UI icons.")
    parser.add_argument("--preview", help="Optional path to save visual contact sheet PNG")
    args = parser.parse_args()

    files = generate_all()
    print(f"Generated {len(files)} UI icon files for {len(ICONS)} icon masters.")
    if args.preview:
        build_contact_sheet(args.preview)
        print(f"Saved preview sheet to {args.preview}")


if __name__ == "__main__":
    main()
