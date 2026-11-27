#!/usr/bin/env python3
"""Generate 2x supersampled Libris-branded (Obsidian & Brass Gold) onboarding PNG illustrations."""

import os
import subprocess
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "onboarding")
os.makedirs(OUT_DIR, exist_ok=True)

BRASS_DARK = (212, 168, 83, 255)        # #D4A853
BRASS_LIGHT = (176, 125, 43, 255)       # #B07D2B
BRASS_GLOW_DARK = (212, 168, 83, 52)
BRASS_GLOW_LIGHT = (176, 125, 43, 40)
TOUCH_BLUE = (56, 189, 248, 255)        # #38BDF8 for interactive touch pulse only

FONT_CANDIDATES = [
    "/usr/share/fonts/google-carlito-fonts/Carlito-Bold.ttf",
    "/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/google-noto/NotoSans-SemiCondensedBold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
]


def resolve_bold_font_path():
    for p in FONT_CANDIDATES:
        if os.path.exists(p):
            return p
    try:
        out = subprocess.check_output(["fc-match", "-f", "%{file}", "Noto Sans:bold"], text=True).strip()
        if out and os.path.exists(out):
            return out
    except Exception:
        pass
    raise RuntimeError("No bold TrueType/OpenType font found for onboarding illustrations!")


BOLD_FONT_PATH = resolve_bold_font_path()


def get_font(size):
    return ImageFont.truetype(BOLD_FONT_PATH, size)


def make_canvas(w, h):
    return Image.new("RGBA", (w * 2, h * 2), (0, 0, 0, 0)), w * 2, h * 2


def save_downsampled(img, filename):
    out = img.resize((img.width // 2, img.height // 2), Image.LANCZOS)
    out.save(os.path.join(OUT_DIR, filename), "PNG")


def add_glow_circle(base, cx, cy, r, color_rgba, blur=36):
    layer = Image.new("RGBA", base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=color_rgba)
    layer = layer.filter(ImageFilter.GaussianBlur(blur))
    return Image.alpha_composite(base, layer)


def draw_rr(d, box, radius, fill=None, outline=None, width=2):
    d.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def theme_palette(dark):
    if dark:
        return {
            "card": (30, 34, 42, 255),
            "card2": (42, 47, 58, 255),
            "bezel": (16, 19, 24, 255),
            "screen": (22, 25, 31, 255),
            "chrome": (12, 14, 18, 255),
            "ink": (240, 236, 228, 255),
            "muted": (146, 142, 134, 255),
            "line": (68, 74, 86, 255),
            "border": (78, 84, 98, 255),
            "accent": BRASS_DARK,
            "accent_soft": (212, 168, 83, 95),
            "accent_tint": (212, 168, 83, 28),
            "glow": BRASS_GLOW_DARK,
        }
    return {
        "card": (255, 253, 249, 255),
        "card2": (240, 235, 225, 255),
        "bezel": (24, 27, 32, 255),
        "screen": (248, 245, 238, 255),
        "chrome": (20, 22, 27, 255),
        "ink": (30, 28, 24, 255),
        "muted": (128, 122, 112, 255),
        "line": (208, 202, 192, 255),
        "border": (212, 204, 190, 255),
        "accent": BRASS_LIGHT,
        "accent_soft": (176, 125, 43, 105),
        "accent_tint": (176, 125, 43, 26),
        "glow": BRASS_GLOW_LIGHT,
    }


def gen_lang(dark):
    img, W, H = make_canvas(280, 280)
    p = theme_palette(dark)
    img = add_glow_circle(img, W // 2, H // 2, 210, p["glow"], blur=55)
    d = ImageDraw.Draw(img)
    cx, cy = W // 2, H // 2

    # Outer halo ring & card circle matching ob_ready style
    d.ellipse((cx - 206, cy - 206, cx + 206, cy + 206), outline=p["accent_soft"], width=6)
    d.ellipse((cx - 168, cy - 168, cx + 168, cy + 168), fill=p["card"], outline=p["accent"], width=10)

    # Crisp geometric globe wireframe inside
    r = 104
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=p["accent"], width=10)
    d.ellipse((cx - 50, cy - r, cx + 50, cy + r), outline=p["accent"], width=9)
    d.line([(cx, cy - r), (cx, cy + r)], fill=p["accent"], width=9)
    d.line([(cx - r, cy), (cx + r, cy)], fill=p["accent"], width=9)
    # Upper and lower latitude chord lines (clipped cleanly inside globe)
    lat_dy = 52
    lat_dx = int((r * r - lat_dy * lat_dy) ** 0.5)
    d.line([(cx - lat_dx, cy - lat_dy), (cx + lat_dx, cy - lat_dy)], fill=p["accent"], width=8)
    d.line([(cx - lat_dx, cy + lat_dy), (cx + lat_dx, cy + lat_dy)], fill=p["accent"], width=8)

    save_downsampled(img, f"ob_lang_{'dark' if dark else 'light'}.png")


def draw_passport_keyboard(d, kx1, ky1, kx2, ky2, fret_color=(122, 130, 142, 255), key_fill=(24, 28, 34, 255), key_edge=(52, 58, 68, 255)):
    """Draws BlackBerry Passport's signature 3-row physical keyboard with horizontal silver frets."""
    kh = ky2 - ky1
    row_h = kh // 3
    kw = kx2 - kx1

    # Top metallic fret separating screen from keyboard
    d.line([(kx1, ky1), (kx2, ky1)], fill=fret_color, width=5)

    # Row 1: 10 keys
    pad_x = 10
    gap = 6
    n1 = 10
    w1 = (kw - pad_x * 2 - gap * (n1 - 1)) / n1
    for i in range(n1):
        x1 = int(kx1 + pad_x + i * (w1 + gap))
        x2 = int(x1 + w1)
        draw_rr(d, (x1, ky1 + 10, x2, ky1 + row_h - 8), 8, fill=key_fill, outline=key_edge, width=2)

    # Fret between Row 1 and Row 2
    y_f1 = ky1 + row_h
    d.line([(kx1, y_f1), (kx2, y_f1)], fill=fret_color, width=4)

    # Row 2: 9 keys (slightly inset like Passport)
    n2 = 9
    pad2 = 20
    w2 = (kw - pad2 * 2 - gap * (n2 - 1)) / n2
    for i in range(n2):
        x1 = int(kx1 + pad2 + i * (w2 + gap))
        x2 = int(x1 + w2)
        draw_rr(d, (x1, y_f1 + 9, x2, y_f1 + row_h - 8), 8, fill=key_fill, outline=key_edge, width=2)

    # Fret between Row 2 and Row 3
    y_f2 = ky1 + row_h * 2
    d.line([(kx1, y_f2), (kx2, y_f2)], fill=fret_color, width=4)

    # Row 3: 3 left keys + wide center Spacebar + 3 right keys
    pad3 = 16
    side_n = 3
    space_w = int(kw * 0.28)
    side_total = (kw - pad3 * 2 - space_w - gap * 2) // 2
    w3 = (side_total - gap * (side_n - 1)) / side_n
    for i in range(side_n):
        x1 = int(kx1 + pad3 + i * (w3 + gap))
        x2 = int(x1 + w3)
        draw_rr(d, (x1, y_f2 + 9, x2, ky2 - 8), 8, fill=key_fill, outline=key_edge, width=2)
    # Center spacebar
    sp_x1 = int(kx1 + pad3 + side_total + gap)
    sp_x2 = sp_x1 + space_w
    draw_rr(d, (sp_x1, y_f2 + 9, sp_x2, ky2 - 8), 8, fill=(30, 35, 42, 255), outline=key_edge, width=2)
    # Right 3 keys
    r_start = sp_x2 + gap
    for i in range(side_n):
        x1 = int(r_start + i * (w3 + gap))
        x2 = int(x1 + w3)
        draw_rr(d, (x1, y_f2 + 9, x2, ky2 - 8), 8, fill=key_fill, outline=key_edge, width=2)


def gen_theme_preview(is_dark_preview):
    img, W, H = make_canvas(340, 420)
    p = theme_palette(is_dark_preview)
    d = ImageDraw.Draw(img)

    # Authentic BlackBerry Passport chassis (wide rectangular body, brushed silver/gunmetal frame)
    bx1, by1, bx2, by2 = 34, 24, W - 34, H - 24
    draw_rr(d, (bx1, by1, bx2, by2), 40, fill=(14, 16, 20, 255), outline=(120, 128, 140, 255), width=7)
    # Inner subtle metallic rim
    draw_rr(d, (bx1 + 8, by1 + 8, bx2 - 8, by2 - 8), 34, fill=(18, 21, 26, 255), outline=(52, 58, 68, 255), width=2)

    # Top earpiece slit + front camera dot
    draw_rr(d, (W // 2 - 52, by1 + 24, W // 2 + 52, by1 + 33), 5, fill=(76, 82, 92, 255))
    d.ellipse((bx2 - 84, by1 + 21, bx2 - 68, by1 + 37), fill=(48, 54, 64, 255))

    # Square 1:1 Passport Screen
    sx1, sy1, sx2 = bx1 + 26, by1 + 56, bx2 - 26
    screen_w = sx2 - sx1
    sy2 = sy1 + int(screen_w * 0.92)
    draw_rr(d, (sx1, sy1, sx2, sy2), 10, fill=p["screen"])

    # BB10 Signature Dark Top TitleBar (kept dark in both Light & Dark Mode)
    d.rectangle((sx1, sy1, sx2, sy1 + 64), fill=p["chrome"])
    draw_rr(d, (sx1 + 22, sy1 + 24, sx1 + 170, sy1 + 40), 8, fill=(238, 234, 226, 255))

    # Book reading lines inside screen
    y = sy1 + 92
    widths = [430, 456, 404, 444, 340, 420]
    for w in widths:
        draw_rr(d, (sx1 + 28, y, min(sx2 - 28, sx1 + 28 + w), y + 14), 7, fill=p["ink"] if w > 380 else p["muted"])
        y += 38

    # Libris Brass progress bar
    draw_rr(d, (sx1 + 28, sy2 - 102, sx2 - 28, sy2 - 90), 6, fill=p["line"])
    draw_rr(d, (sx1 + 28, sy2 - 102, sx1 + 290, sy2 - 90), 6, fill=p["accent"])

    # BB10 Signature Dark Bottom ActionBar
    d.rectangle((sx1, sy2 - 66, sx2, sy2), fill=p["chrome"])
    for cx in (sx1 + 100, sx2 - 100):
        d.ellipse((cx - 14, sy2 - 47, cx + 14, sy2 - 19), fill=(95, 102, 114, 255))
    d.ellipse((W // 2 - 16, sy2 - 49, W // 2 + 16, sy2 - 17), fill=p["accent"])

    # BlackBerry Passport 3-Row Physical Keyboard directly below screen
    draw_passport_keyboard(d, sx1 - 4, sy2 + 16, sx2 + 4, by2 - 22)

    save_downsampled(img, f"ob_theme_{'dark' if is_dark_preview else 'light'}_preview.png")


def gen_books(dark):
    img, W, H = make_canvas(640, 340)
    p = theme_palette(dark)
    d = ImageDraw.Draw(img)

    # 3 Curved transfer paths with crisp arrowheads pointing to center BlackBerry Passport
    d.arc((210, 170, 530, 440), 210, 322, fill=p["accent_soft"], width=7)
    d.polygon([(504, 256), (474, 240), (484, 272)], fill=p["accent"])

    d.line((W // 2, 162, W // 2, 244), fill=p["accent"], width=7)
    d.polygon([(W // 2, 256), (W // 2 - 16, 230), (W // 2 + 16, 230)], fill=p["accent"])

    d.arc((750, 170, 1070, 440), 218, 330, fill=p["accent_soft"], width=7)
    d.polygon([(776, 256), (806, 240), (796, 272)], fill=p["accent"])

    # Center BlackBerry Passport silhouette (square screen + 3-row fret keyboard)
    px1, py1, px2, py2 = W // 2 - 148, 266, W // 2 + 148, 646
    draw_rr(d, (px1, py1, px2, py2), 28, fill=(14, 16, 20, 255), outline=p["accent"], width=5)
    # Square screen
    sx1, sy1, sx2, sy2 = px1 + 16, py1 + 26, px2 - 16, py1 + 258
    draw_rr(d, (sx1, sy1, sx2, sy2), 8, fill=p["screen"])
    # 2x2 book covers inside square screen
    for gx in range(2):
        for gy in range(2):
            cx1 = sx1 + 24 + gx * 116
            cy1 = sy1 + 18 + gy * 102
            draw_rr(d, (cx1, cy1, cx1 + 98, cy1 + 88), 8, fill=p["accent"] if (gx + gy) == 0 else p["card2"], outline=p["border"], width=2)
    # Compact 3-row Passport keyboard frets
    ky1, ky2 = sy2 + 10, py2 - 14
    kh = (ky2 - ky1) // 3
    for r in range(3):
        yf = ky1 + r * kh
        d.line([(sx1, yf), (sx2, yf)], fill=(116, 124, 136, 255), width=3)
        for c in range(7):
            kx = sx1 + 6 + c * 36
            draw_rr(d, (kx, yf + 5, kx + 28, yf + kh - 4), 4, fill=(28, 32, 38, 255))

    # Node 1: Left SD Card badge
    draw_rr(d, (90, 270, 250, 430), 34, fill=p["card"], outline=p["border"], width=4)
    d.polygon([(140, 306), (192, 306), (208, 324), (208, 392), (140, 392)], fill=p["accent"])
    for k in range(3):
        d.rectangle((150 + k * 14, 316, 158 + k * 14, 334), fill=p["card"])

    # Node 2: Top Wi-Fi badge
    draw_rr(d, (W // 2 - 80, 18, W // 2 + 80, 158), 34, fill=p["card"], outline=p["accent"], width=4)
    cx, cy = W // 2, 122
    for rr in (26, 52, 78):
        d.arc((cx - rr, cy - rr, cx + rr, cy + rr), 225, 315, fill=p["accent"], width=7)
    d.ellipse((cx - 8, cy - 8, cx + 8, cy + 8), fill=p["accent"])

    # Node 3: Right OPDS Cloud badge
    draw_rr(d, (W - 250, 270, W - 90, 430), 34, fill=p["card"], outline=p["border"], width=4)
    d.ellipse((W - 216, 336, W - 160, 388), fill=p["accent"])
    d.ellipse((W - 190, 316, W - 128, 378), fill=p["accent"])
    d.ellipse((W - 156, 338, W - 114, 388), fill=p["accent"])
    d.rectangle((W - 190, 358, W - 134, 388), fill=p["accent"])

    save_downsampled(img, f"ob_books_{'dark' if dark else 'light'}.png")


def gen_gesture_frame(dark):
    img, W, H = make_canvas(600, 340)
    p = theme_palette(dark)
    d = ImageDraw.Draw(img)

    # Sleek rounded BB10 device frame
    draw_rr(d, (16, 12, W - 16, H - 12), 46, fill=p["bezel"], outline=p["border"], width=4)
    draw_rr(d, (38, 34, W - 38, H - 34), 28, fill=p["screen"])

    # Clean book lines in middle
    for row in range(6):
        y = 140 + row * 70
        draw_rr(d, (235, y, W - 235 if row != 5 else W - 365, y + 14), 7, fill=p["line"])

    # Subtle translucent edge tap zones (alpha=26, crisp outline)
    overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay)
    draw_rr(od, (52, 86, 178, H - 86), 22, fill=p["accent_tint"], outline=p["accent_soft"], width=3)
    draw_rr(od, (W - 178, 86, W - 52, H - 86), 22, fill=p["accent_tint"], outline=p["accent_soft"], width=3)
    img = Image.alpha_composite(img, overlay)

    save_downsampled(img, f"ob_gesture_frame_{'dark' if dark else 'light'}.png")


def gen_touch_ring():
    img, W, H = make_canvas(96, 96)
    img = add_glow_circle(img, W // 2, H // 2, 54, (56, 189, 248, 90), blur=16)
    d = ImageDraw.Draw(img)
    cx, cy = W // 2, H // 2
    d.ellipse((cx - 64, cy - 64, cx + 64, cy + 64), outline=(56, 189, 248, 175), width=5)
    d.ellipse((cx - 40, cy - 40, cx + 40, cy + 40), fill=(56, 189, 248, 105), outline=TOUCH_BLUE, width=5)
    d.ellipse((cx - 12, cy - 12, cx + 12, cy + 12), fill=(255, 255, 255, 235))
    save_downsampled(img, "ob_touch_ring.png")


def gen_tools(dark):
    img, W, H = make_canvas(640, 360)
    p = theme_palette(dark)
    img = add_glow_circle(img, W // 2, H // 2, 220, p["glow"], blur=60)
    d = ImageDraw.Draw(img)

    # Reading page card in center
    draw_rr(d, (210, 74, W - 210, H - 64), 38, fill=p["card"], outline=p["border"], width=4)

    # Top lines + non-overlapping highlighted passage on left/center
    draw_rr(d, (272, 146, 650, 166), 10, fill=p["muted"])
    draw_rr(d, (262, 206, 650, 260), 14, fill=p["accent_tint"], outline=p["accent"], width=3)
    draw_rr(d, (278, 224, 632, 242), 9, fill=p["ink"])
    draw_rr(d, (272, 298, 720, 318), 10, fill=p["muted"])
    draw_rr(d, (272, 354, 660, 374), 10, fill=p["muted"])

    f_lbl = get_font(52)

    # Floating Dictionary Tooltip pill (Top-Right, cleanly outside highlight bar)
    draw_rr(d, (690, 108, 1150, 256), 32, fill=p["card2"], outline=p["accent"], width=4)
    d.text((920, 160), "Aa -> Vi", font=f_lbl, fill=p["accent"], anchor="mm")
    draw_rr(d, (746, 212, 1094, 228), 8, fill=p["ink"])

    # Floating Quote Card pill (Bottom-Left)
    draw_rr(d, (124, 424, 520, 566), 32, fill=p["card2"], outline=p["accent"], width=4)
    d.text((322, 495), "\"Quote\"", font=f_lbl, fill=p["accent"], anchor="mm")

    # Floating TTS soundwave pill (Bottom-Right)
    draw_rr(d, (760, 424, 1130, 566), 32, fill=p["card2"], outline=p["accent"], width=4)
    bars = [38, 72, 96, 56, 82, 44]
    for idx, bh in enumerate(bars):
        bx = 824 + idx * 42
        by1 = 495 - bh // 2
        by2 = 495 + bh // 2
        draw_rr(d, (bx, by1, bx + 20, by2), 10, fill=p["accent"])

    save_downsampled(img, f"ob_tools_{'dark' if dark else 'light'}.png")


def gen_ready(dark):
    img, W, H = make_canvas(280, 280)
    p = theme_palette(dark)
    img = add_glow_circle(img, W // 2, H // 2, 220, p["glow"], blur=55)
    d = ImageDraw.Draw(img)
    cx, cy = W // 2, H // 2
    d.ellipse((cx - 210, cy - 210, cx + 210, cy + 210), outline=p["accent_soft"], width=6)
    d.ellipse((cx - 165, cy - 165, cx + 165, cy + 165), fill=p["accent"])
    d.line([(cx - 72, cy + 6), (cx - 18, cy + 62), (cx + 82, cy - 52)], fill=(255, 255, 255, 255), width=26, joint="curve")
    save_downsampled(img, f"ob_ready_{'dark' if dark else 'light'}.png")


if __name__ == "__main__":
    print(f"Using bold font: {BOLD_FONT_PATH}")
    for dark in (True, False):
        gen_lang(dark)
        gen_theme_preview(dark)
        gen_books(dark)
        gen_gesture_frame(dark)
        gen_tools(dark)
        gen_ready(dark)
    gen_touch_ring()
    files = sorted(os.listdir(OUT_DIR))
    print(f"Generated {len(files)} illustrations cleanly.")
