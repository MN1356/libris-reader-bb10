import os
import math
from PIL import Image, ImageDraw

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STATS_DIR = os.path.join(BASE_DIR, "assets", "stats")
RINGS_DIR = os.path.join(STATS_DIR, "rings")

os.makedirs(STATS_DIR, exist_ok=True)
os.makedirs(RINGS_DIR, exist_ok=True)


def make_rounded_9slice(name, size, radius, fill_rgba, border_rgba=None, border_px=1, slice_margin=14):
    scale = 4
    w = h = size * scale
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    r = radius * scale
    if border_rgba is not None and border_px > 0:
        draw.rounded_rectangle([0, 0, w - 1, h - 1], radius=r, fill=border_rgba)
        bp = border_px * scale
        inner_r = max(2, r - bp)
        draw.rounded_rectangle([bp, bp, w - 1 - bp, h - 1 - bp], radius=inner_r, fill=fill_rgba)
    else:
        draw.rounded_rectangle([0, 0, w - 1, h - 1], radius=r, fill=fill_rgba)

    out_png = img.resize((size, size), Image.LANCZOS)
    png_name = f"{name}.png"
    out_png.save(os.path.join(STATS_DIR, png_name))

    amd_path = os.path.join(STATS_DIR, f"{name}.amd")
    amd_content = (
        "#RimCascadesAssetMetaData version=1.0\n"
        f'source: "{png_name}"\n'
        f"sliceMargins: {slice_margin} {slice_margin} {slice_margin} {slice_margin}\n"
    )
    with open(amd_path, "w", encoding="utf-8") as f:
        f.write(amd_content)


def make_ring(pct, theme):
    # 160x160 final size, 4x supersampled (640x640)
    size = 160
    scale = 4
    S = size * scale
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    if theme == "dark":
        track_col = (42, 47, 56, 255)      # #2A2F38
        fill_col = (212, 168, 83, 255)     # #D4A853
    else:
        track_col = (221, 214, 200, 255)   # #DDD6C8
        fill_col = (156, 107, 25, 255)     # #9C6B19

    stroke = 14 * scale
    pad = 10 * scale
    bbox = [pad, pad, S - pad, S - pad]

    # Full background track circle
    draw.arc(bbox, start=0, end=360, fill=track_col, width=stroke)

    if pct > 0:
        start_deg = -90
        end_deg = -90 + (360.0 * pct / 100.0)
        draw.arc(bbox, start=start_deg, end=end_deg, fill=fill_col, width=stroke)

        # Rounded end caps for smooth finish
        if pct < 100:
            r_center = (S - 2 * pad - stroke) / 2.0 + stroke / 2.0
            cx = cy = S / 2.0
            cap_r = stroke / 2.0
            for ang_deg in (start_deg, end_deg):
                rad = math.radians(ang_deg)
                px = cx + (S / 2.0 - pad - stroke / 2.0) * math.cos(rad)
                py = cy + (S / 2.0 - pad - stroke / 2.0) * math.sin(rad)
                draw.ellipse([px - cap_r, py - cap_r, px + cap_r, py + cap_r], fill=fill_col)

    out = img.resize((size, size), Image.LANCZOS)
    out.save(os.path.join(RINGS_DIR, f"ring_{theme}_{pct}.png"))


def main():
    # Cards (48x48, r=10, slice=14)
    make_rounded_9slice("card_dark", 48, 10, (26, 29, 35, 255), (42, 47, 56, 255), 1, 14)
    make_rounded_9slice("card_light", 48, 10, (255, 255, 255, 255), (221, 214, 200, 255), 1, 14)

    # Calendar cells (40x40, r=7, slice=12)
    make_rounded_9slice("cell_dark", 40, 7, (26, 29, 35, 255), (42, 47, 56, 255), 1, 12)
    make_rounded_9slice("cell_light", 40, 7, (255, 255, 255, 255), (221, 214, 200, 255), 1, 12)

    make_rounded_9slice("cell_sel_dark", 40, 7, (38, 31, 21, 255), (212, 168, 83, 255), 2, 12)
    make_rounded_9slice("cell_sel_light", 40, 7, (247, 235, 211, 255), (156, 107, 25, 255), 2, 12)

    make_rounded_9slice("cell_today_dark", 40, 7, (26, 29, 35, 255), (212, 168, 83, 255), 2, 12)
    make_rounded_9slice("cell_today_light", 40, 7, (255, 255, 255, 255), (156, 107, 25, 255), 2, 12)

    make_rounded_9slice("cell_out_dark", 40, 7, (14, 12, 10, 255), (28, 31, 38, 255), 1, 12)
    make_rounded_9slice("cell_out_light", 40, 7, (232, 227, 216, 255), (210, 203, 190, 255), 1, 12)

    # Pill Accent (Edit button / badges) (40x40, r=8, slice=12)
    make_rounded_9slice("pill_accent_dark", 40, 8, (38, 31, 21, 255), (140, 103, 46, 255), 1, 12)
    make_rounded_9slice("pill_accent_light", 40, 8, (247, 235, 211, 255), (181, 127, 42, 255), 1, 12)

    # Primary Button (40x40, r=9, slice=12)
    make_rounded_9slice("btn_primary_dark", 40, 9, (212, 168, 83, 255), None, 0, 12)
    make_rounded_9slice("btn_primary_light", 40, 9, (156, 107, 25, 255), None, 0, 12)

    # Circular Goal Rings (0..100 in steps of 5)
    for pct in range(0, 105, 5):
        make_ring(pct, "dark")
        make_ring(pct, "light")

    print("Generated 9-slice AMD/PNG assets and 42 ring PNGs in", STATS_DIR)


if __name__ == "__main__":
    main()
