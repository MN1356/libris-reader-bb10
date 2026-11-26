#!/usr/bin/env python3
"""
Libris Minimalist Splash Screen Generator
Creates clean, elegant splash screens with ONLY:
- Logo (sharp book & ribbon emblem)
- Application Name ('Libris')
- Native Loading Effect (smooth circular spinner)
Target resolutions:
- 720x720   (Classic, Q10, Q5)
- 1440x1440 (Passport)
- 720x1280  (Z30, Leap, Z3)
- 768x1280  (Z10, P'9982)
"""

import math
import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

FONT_PATH = "/usr/share/fonts/open-sans/OpenSans-Bold.ttf"
if not os.path.exists(FONT_PATH):
    FONT_PATH = "/usr/share/fonts/google-noto/NotoSans-Bold.ttf"

def extract_clean_emblem():
    src = Image.open("icon-1024.png").convert("RGBA")
    crop_box = (180, 240, 844, 850)
    emblem_raw = src.crop(crop_box)
    ew, eh = emblem_raw.size

    clean_emblem = Image.new("RGBA", (ew, eh), (0, 0, 0, 0))
    ps = emblem_raw.load()
    pd = clean_emblem.load()

    for y in range(eh):
        for x in range(ew):
            r, g, b, a = ps[x, y]
            max_c = max(r, g, b)
            if max_c < 55:
                pd[x, y] = (0, 0, 0, 0)
            elif max_c < 90:
                ratio = (max_c - 55) / 35.0
                pd[x, y] = (r, g, b, int(255 * ratio))
            else:
                pd[x, y] = (r, g, b, 255)
    return clean_emblem

def draw_loading_spinner(size, stroke_width, active_color=(0, 200, 255), track_color=(255, 255, 255, 30)):
    # Render at 4x for super sampling antialiasing
    scale = 4
    canvas_size = size * scale
    sw = stroke_width * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    margin = sw
    box = [margin, margin, canvas_size - margin, canvas_size - margin]

    # Background track ring (full 360 circle)
    draw.ellipse(box, outline=track_color, width=sw)

    # Active spinning arc (260 degrees arc with gradient)
    start_deg = -30
    total_arc = 250
    steps = 50
    radius = (canvas_size - 2 * margin) / 2.0
    cx = canvas_size / 2.0
    cy = canvas_size / 2.0

    for i in range(steps):
        angle1 = start_deg + (total_arc * i / steps)
        angle2 = start_deg + (total_arc * (i + 1.2) / steps)
        frac = i / float(steps)
        # Alpha from 30 to 255
        alpha = int(30 + 225 * (frac ** 1.8))
        # Color slight shift to bright white at tip
        r = int(active_color[0] + (255 - active_color[0]) * frac * 0.7)
        g = int(active_color[1] + (255 - active_color[1]) * frac * 0.7)
        b = int(active_color[2] + (255 - active_color[2]) * frac * 0.7)
        draw.arc(box, start=angle1, end=angle2, fill=(r, g, b, alpha), width=sw)

    # Glowing tip at the head of the arc
    head_angle_rad = math.radians(start_deg + total_arc)
    hx = cx + radius * math.cos(head_angle_rad)
    hy = cy + radius * math.sin(head_angle_rad)
    head_r = sw / 2.0
    draw.ellipse([hx - head_r, hy - head_r, hx + head_r, hy + head_r], fill=(255, 255, 255, 255))

    # Downsample with Lanczos for perfect smooth curve
    spinner = img.resize((size, size), Image.Resampling.LANCZOS)
    return spinner

def create_splash(width, height, emblem):
    # Pure AMOLED Black
    splash = Image.new("RGBA", (width, height), (0, 0, 0, 255))

    # Scale factor relative to 720 baseline
    scale = width / 720.0
    if height == 1280:
        scale = 1.05

    # 1. Logo Emblem
    logo_w = int(170 * scale)
    logo_h = int(emblem.height * (logo_w / float(emblem.width)))
    logo_resized = emblem.resize((logo_w, logo_h), Image.Resampling.LANCZOS)

    # 2. Typography
    font_size = int(34 * scale)
    font = ImageFont.truetype(FONT_PATH, font_size)
    app_name = "Libris"

    # Compute text bbox
    dummy_draw = ImageDraw.Draw(splash)
    bbox = dummy_draw.textbbox((0, 0), app_name, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]

    # 3. Loading Spinner
    spinner_size = int(38 * scale)
    spinner_stroke = max(2, int(3 * scale))
    spinner = draw_loading_spinner(spinner_size, spinner_stroke)

    # Vertical Layout & Spacings
    spacing_logo_text = int(22 * scale)
    spacing_text_spinner = int(38 * scale)

    total_group_h = logo_h + spacing_logo_text + text_h + spacing_text_spinner + spinner_size

    # Position group centered (or slightly above optical center for tall aspect ratio)
    start_y = (height - total_group_h) // 2
    if height > width:
        # Move slightly up towards optical center on portrait displays
        start_y = int(start_y * 0.92)

    # Draw Emblem
    logo_x = (width - logo_w) // 2
    logo_y = start_y
    splash.paste(logo_resized, (logo_x, logo_y), logo_resized)

    # Draw Text
    draw = ImageDraw.Draw(splash)
    text_x = (width - text_w) // 2 - bbox[0]
    text_y = logo_y + logo_h + spacing_logo_text - bbox[1]
    draw.text((text_x, text_y), app_name, font=font, fill=(255, 255, 255, 255))

    # Draw Loading Spinner
    spinner_x = (width - spinner_size) // 2
    spinner_y = logo_y + logo_h + spacing_logo_text + text_h + spacing_text_spinner
    splash.paste(spinner, (spinner_x, spinner_y), spinner)

    return splash.convert("RGB")

def main():
    print("[SPLASH] Extracting clean emblem from icon-1024.png...")
    emblem = extract_clean_emblem()

    targets = [
        ("splash-720x720.png", 720, 720, "Classic / Q10 / Q5"),
        ("splash-1440x1440.png", 1440, 1440, "Passport"),
        ("splash-720x1280.png", 720, 1280, "Z30 / Leap / Z3"),
        ("splash-768x1280.png", 768, 1280, "Z10 / P'9982"),
    ]

    for fname, w, h, desc in targets:
        print(f"[SPLASH] Generating {fname} ({w}x{h} - {desc})...")
        img = create_splash(w, h, emblem)
        img.save(fname, "PNG", optimize=True)
        print(f"  -> Saved {fname} ({os.path.getsize(fname):,} bytes)")

    print("[SPLASH] All splash screens generated successfully with Logo + Name + Loading Effect ONLY!")

if __name__ == "__main__":
    main()
