#!/usr/bin/env python3
"""
BerryLeaf Brand Identity Generator
Generates:
1. High-res app icons for all BlackBerry 10 resolutions based on the new Master Logo:
   Open book + Reading figure with overlapping emerald leaf in front.
2. Splash screens for all BlackBerry 10 screen targets:
   Luminescent emblem with emerald backlight and loading ring on AMOLED black.
3. Custom UI icons:
   - ic_library.png (3 books standing side-by-side)
   - ic_mode_light.png (Sun / day mode)
   - ic_mode_dark.png (Moon / night mode)
   - ic_mode_sepia.png (Parchment / sepia mode)
"""

import math
import os
import subprocess
from PIL import Image, ImageDraw, ImageFont, ImageFilter

SVG_LIGHT = "assets/icons/berryleaf_logo.svg"
SVG_DARK = "assets/icons/berryleaf_logo_dark.svg"

def get_vector_emblem(size=1024, dark_mode=False):
    """Renders the master vector SVG logo at any resolution using ImageMagick."""
    svg_path = SVG_DARK if dark_mode else SVG_LIGHT
    out_tmp = f"/tmp/emblem_{size}_{'dark' if dark_mode else 'light'}.png"
    subprocess.run(["magick", "-background", "none", svg_path, "-resize", f"{size}x{size}", out_tmp], check=True)
    return Image.open(out_tmp).convert("RGBA")

def create_app_icon(size=1024, style="light"):
    """
    Creates master BerryLeaf app icon.
    style='light': Premium pearl-white squircle tile with dark navy & emerald emblem (Option A).
    style='dark': Deep AMOLED slate squircle tile with luminescent emblem (Option B).
    """
    icon = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    radius = int(size * 0.22)
    mask = Image.new("L", (size, size), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([0, 0, size, size], radius=radius, fill=255)

    if style == "light":
        # Premium Pearl-White gradient background: #FFFFFF to #F1F5F9
        bg = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        bdraw = ImageDraw.Draw(bg)
        for y in range(size):
            t = y / float(size)
            r = int(255 * (1 - t) + 241 * t)
            g = int(255 * (1 - t) + 245 * t)
            b = int(255 * (1 - t) + 249 * t)
            bdraw.line([(0, y), (size, y)], fill=(r, g, b, 255))
        bg.putalpha(mask)
        icon.paste(bg, (0, 0), bg)

        # Subtle outer border and inner bevel
        idraw = ImageDraw.Draw(icon)
        idraw.rounded_rectangle([2, 2, size - 2, size - 2], radius=radius, outline=(203, 213, 225, 255), width=max(2, int(size * 0.005)))
        idraw.rounded_rectangle([6, 6, size - 6, size - 6], radius=radius - 4, outline=(255, 255, 255, 200), width=max(2, int(size * 0.004)))

        # Center Light Emblem
        lw = int(size * 0.74)
        emblem = get_vector_emblem(lw, dark_mode=False)
        lh = int(lw * emblem.height / emblem.width)
        emblem_resized = emblem.resize((lw, lh), Image.Resampling.LANCZOS)
        lx = (size - lw) // 2
        ly = (size - lh) // 2
        icon.paste(emblem_resized, (lx, ly), emblem_resized)

    else:
        # Deep Slate Navy gradient background: #121820 to #060A0E
        bg = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        bdraw = ImageDraw.Draw(bg)
        for y in range(size):
            t = y / float(size)
            r = int(18 * (1 - t) + 8 * t)
            g = int(28 * (1 - t) + 14 * t)
            b = int(36 * (1 - t) + 20 * t)
            bdraw.line([(0, y), (size, y)], fill=(r, g, b, 255))
        bg.putalpha(mask)
        icon.paste(bg, (0, 0), bg)

        # Subtle emerald backlight glow
        glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        gdraw = ImageDraw.Draw(glow)
        gdraw.ellipse([size * 0.2, size * 0.2, size * 0.8, size * 0.8], fill=(16, 185, 129, 65))
        glow = glow.filter(ImageFilter.GaussianBlur(radius=int(size * 0.07)))
        icon.paste(glow, (0, 0), glow)

        # Subtle dark border
        idraw = ImageDraw.Draw(icon)
        idraw.rounded_rectangle([2, 2, size - 2, size - 2], radius=radius, outline=(30, 41, 59, 240), width=max(2, int(size * 0.005)))
        idraw.rounded_rectangle([5, 5, size - 5, size - 5], radius=radius - 3, outline=(16, 185, 129, 60), width=max(1, int(size * 0.003)))

        # Center Dark Luminescent Emblem
        lw = int(size * 0.74)
        emblem = get_vector_emblem(lw, dark_mode=True)
        lh = int(lw * emblem.height / emblem.width)
        emblem_resized = emblem.resize((lw, lh), Image.Resampling.LANCZOS)
        lx = (size - lw) // 2
        ly = (size - lh) // 2
        icon.paste(emblem_resized, (lx, ly), emblem_resized)

    return icon

def create_3books_icon(size=96, dark_mode=False):
    """Creates an icon of 3 books standing neatly side by side on a shelf for Library tab."""
    scale = 4
    canvas_size = size * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    pad_x = int(canvas_size * 0.18)
    bot_y = int(canvas_size * 0.84)
    shelf_h = int(canvas_size * 0.05)
    shelf_color = (255, 255, 255, 200) if not dark_mode else (226, 232, 240, 240)
    draw.rectangle([pad_x - int(scale * 4), bot_y, canvas_size - pad_x + int(scale * 4), bot_y + shelf_h], fill=shelf_color)

    # Book 1 (Left: Emerald Leaf Green)
    b1_x1 = int(canvas_size * 0.22)
    b1_w = int(canvas_size * 0.16)
    b1_h = int(canvas_size * 0.52)
    b1_y1 = bot_y - b1_h
    draw.rounded_rectangle([b1_x1, b1_y1, b1_x1 + b1_w, bot_y], radius=int(scale * 3),
                           fill=(16, 185, 129, 255), outline=shelf_color, width=int(scale * 2))
    draw.line([(b1_x1 + int(scale * 3), b1_y1 + int(b1_h * 0.25)), (b1_x1 + b1_w - int(scale * 3), b1_y1 + int(b1_h * 0.25))],
              fill=(255, 255, 255, 220), width=int(scale * 2))
    draw.line([(b1_x1 + int(scale * 3), b1_y1 + int(b1_h * 0.35)), (b1_x1 + b1_w - int(scale * 3), b1_y1 + int(b1_h * 0.35))],
              fill=(255, 255, 255, 220), width=int(scale * 2))

    # Book 2 (Middle: Ocean Cyan)
    b2_x1 = b1_x1 + b1_w + int(scale * 5)
    b2_w = int(canvas_size * 0.17)
    b2_h = int(canvas_size * 0.60)
    b2_y1 = bot_y - b2_h
    draw.rounded_rectangle([b2_x1, b2_y1, b2_x1 + b2_w, bot_y], radius=int(scale * 3),
                           fill=(2, 132, 199, 255), outline=shelf_color, width=int(scale * 2))
    ribbon_w = int(scale * 6)
    ribbon_x = b2_x1 + (b2_w - ribbon_w) // 2
    draw.rectangle([ribbon_x, b2_y1 + int(scale * 8), ribbon_x + ribbon_w, b2_y1 + int(b2_h * 0.45)],
                   fill=(251, 191, 36, 255))

    # Book 3 (Right: Violet)
    b3_x1 = b2_x1 + b2_w + int(scale * 5)
    b3_w = int(canvas_size * 0.16)
    b3_h = int(canvas_size * 0.46)
    b3_y1 = bot_y - b3_h
    draw.rounded_rectangle([b3_x1, b3_y1, b3_x1 + b3_w, bot_y], radius=int(scale * 3),
                           fill=(139, 92, 246, 255), outline=shelf_color, width=int(scale * 2))
    draw.line([(b3_x1 + int(scale * 3), b3_y1 + int(b3_h * 0.2)), (b3_x1 + b3_w - int(scale * 3), b3_y1 + int(b3_h * 0.2))],
              fill=(255, 255, 255, 220), width=int(scale * 2))
    draw.line([(b3_x1 + int(scale * 3), b3_y1 + int(b3_h * 0.3)), (b3_x1 + b3_w - int(scale * 3), b3_y1 + int(b3_h * 0.3))],
              fill=(255, 255, 255, 220), width=int(scale * 2))

    return img.resize((size, size), Image.Resampling.LANCZOS)

def create_mode_light_icon(size=96):
    """Clean Sun / Light Mode Icon"""
    scale = 4
    canvas_size = size * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = canvas_size // 2, canvas_size // 2
    r_core = int(canvas_size * 0.19)
    sw = int(scale * 5)
    draw.ellipse([cx - r_core, cy - r_core, cx + r_core, cy + r_core], fill=(255, 255, 255, 255))
    ray_inner = int(canvas_size * 0.27)
    ray_outer = int(canvas_size * 0.39)
    for i in range(8):
        angle = i * (math.pi / 4.0)
        x1 = cx + int(ray_inner * math.cos(angle))
        y1 = cy + int(ray_inner * math.sin(angle))
        x2 = cx + int(ray_outer * math.cos(angle))
        y2 = cy + int(ray_outer * math.sin(angle))
        draw.line([(x1, y1), (x2, y2)], fill=(255, 255, 255, 255), width=sw)
    return img.resize((size, size), Image.Resampling.LANCZOS)

def create_mode_dark_icon(size=96):
    """Clean Crescent Moon / Dark Mode Icon"""
    scale = 4
    canvas_size = size * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    cx, cy = canvas_size // 2, canvas_size // 2
    r = int(canvas_size * 0.34)
    moon = Image.new("L", (canvas_size, canvas_size), 0)
    mdraw = ImageDraw.Draw(moon)
    mdraw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)
    cut_x = cx + int(r * 0.42)
    cut_y = cy - int(r * 0.32)
    cut_r = int(r * 0.82)
    mdraw.ellipse([cut_x - cut_r, cut_y - cut_r, cut_x + cut_r, cut_y + cut_r], fill=0)
    surf = Image.new("RGBA", (canvas_size, canvas_size), (255, 255, 255, 255))
    surf.putalpha(moon)
    img.paste(surf, (0, 0), surf)
    sdraw = ImageDraw.Draw(img)
    sx = cx + int(r * 0.35)
    sy = cy + int(r * 0.38)
    star_r = int(scale * 8)
    sdraw.ellipse([sx - star_r, sy - star_r, sx + star_r, sy + star_r], fill=(255, 255, 255, 240))
    return img.resize((size, size), Image.Resampling.LANCZOS)

def create_mode_sepia_icon(size=96):
    """Clean Parchment / Sepia Mode Icon"""
    scale = 4
    canvas_size = size * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = canvas_size // 2, canvas_size // 2
    w = int(canvas_size * 0.58)
    h = int(canvas_size * 0.68)
    x1, y1 = cx - w // 2, cy - h // 2
    fold_size = int(canvas_size * 0.18)
    pts = [(x1, y1), (x1 + w - fold_size, y1), (x1 + w, y1 + fold_size), (x1 + w, y1 + h), (x1, y1 + h)]
    draw.polygon(pts, fill=(244, 236, 216, 255), outline=(217, 184, 137, 255))
    fold_pts = [(x1 + w - fold_size, y1), (x1 + w - fold_size, y1 + fold_size), (x1 + w, y1 + fold_size)]
    draw.polygon(fold_pts, fill=(217, 184, 137, 255))
    line_y = y1 + int(h * 0.35)
    line_spacing = int(h * 0.14)
    line_pad = int(w * 0.16)
    sw = int(scale * 4)
    for _ in range(3):
        draw.line([(x1 + line_pad, line_y), (x1 + w - line_pad, line_y)], fill=(138, 109, 59, 220), width=sw)
        line_y += line_spacing
    return img.resize((size, size), Image.Resampling.LANCZOS)

def draw_loading_spinner(size, stroke_width, active_color=(16, 185, 129), track_color=(255, 255, 255, 30)):
    scale = 4
    canvas_size = size * scale
    sw = stroke_width * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    margin = sw
    box = [margin, margin, canvas_size - margin, canvas_size - margin]
    draw.ellipse(box, outline=track_color, width=sw)
    steps = 90
    arc_span = 260.0
    start_angle = -90.0
    cx = canvas_size / 2.0
    cy = canvas_size / 2.0
    radius = (canvas_size - 2 * margin) / 2.0
    for i in range(steps):
        t = i / float(steps)
        angle_deg = start_angle + t * arc_span
        angle_rad = math.radians(angle_deg)
        x = cx + radius * math.cos(angle_rad)
        y = cy + radius * math.sin(angle_rad)
        alpha = int(40 + 215 * (t ** 1.3))
        r, g, b = active_color
        dot_r = sw / 2.0
        draw.ellipse([x - dot_r, y - dot_r, x + dot_r, y + dot_r], fill=(r, g, b, alpha))
    return img.resize((size, size), Image.Resampling.LANCZOS)

def create_splash(width, height):
    """
    Creates minimalist, premium splash screen with ONLY:
    - Master Luminescent BerryLeaf Logo Emblem
    - Soft emerald ambient backlight
    - Smooth Emerald Loading Ring
    No slogans, no subtitles, pure brand elegance.
    """
    splash = Image.new("RGBA", (width, height), (0, 0, 0, 255))
    scale = width / 720.0

    # 1. Luminescent Dark-Theme Emblem
    logo_w = int(240 * scale)
    emblem = get_vector_emblem(logo_w, dark_mode=True)
    logo_h = int(logo_w * emblem.height / emblem.width)
    logo_resized = emblem.resize((logo_w, logo_h), Image.Resampling.LANCZOS)

    # 2. Loading Spinner Ring
    spinner_size = int(42 * scale)
    spinner_stroke = max(2, int(3.5 * scale))
    spinner = draw_loading_spinner(spinner_size, spinner_stroke)

    spacing = int(48 * scale)
    total_group_h = logo_h + spacing + spinner_size

    start_y = (height - total_group_h) // 2
    if height > width:
        start_y = int((height - total_group_h) * 0.46)

    logo_x = (width - logo_w) // 2
    logo_y = start_y

    # Subtle emerald ambient glow behind emblem
    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(glow)
    gdraw.ellipse([logo_x - int(30 * scale), logo_y - int(30 * scale),
                   logo_x + logo_w + int(30 * scale), logo_y + logo_h + int(30 * scale)],
                  fill=(16, 185, 129, 50))
    glow = glow.filter(ImageFilter.GaussianBlur(radius=int(40 * scale)))
    splash.paste(glow, (0, 0), glow)

    splash.paste(logo_resized, (logo_x, logo_y), logo_resized)

    spinner_x = (width - spinner_size) // 2
    spinner_y = logo_y + logo_h + spacing
    splash.paste(spinner, (spinner_x, spinner_y), spinner)

    return splash.convert("RGB")

def main():
    import sys
    style_choice = "dark" # Default dark for BB10
    if "--light" in sys.argv:
        style_choice = "light"

    print("--> Generating Option A (Pearl-White) and Option B (Dark Slate) app icons...")
    icon_light_1024 = create_app_icon(1024, style="light")
    icon_dark_1024 = create_app_icon(1024, style="dark")

    icon_light_1024.save("icon-1024_light.png")
    icon_dark_1024.save("icon-1024_dark.png")

    primary_icon = icon_light_1024 if style_choice == "light" else icon_dark_1024
    primary_icon.save("icon-1024.png")

    icon_sizes = [
        ("icon-358x358.png", 358),
        ("icon-144x144.png", 144),
        ("icon.png", 144),
        ("icon-114x114.png", 114),
        ("icon-110x110.png", 110),
        ("icon-96x96.png", 96),
        ("icon-90x90.png", 90),
        ("icon-86x86.png", 86)
    ]
    for filename, sz in icon_sizes:
        print(f"    Writing {filename} ({sz}x{sz})...")
        resized = primary_icon.resize((sz, sz), Image.Resampling.LANCZOS)
        resized.save(filename)

    # In-app dynamic theme icons (Option A for light, Option B for dark)
    app_light_96 = icon_light_1024.resize((96, 96), Image.Resampling.LANCZOS)
    app_light_96.save("assets/icons/ic_app_light.png")
    
    app_dark_96 = icon_dark_1024.resize((96, 96), Image.Resampling.LANCZOS)
    app_dark_96.save("assets/icons/ic_app_dark.png")

    # Default fallback
    app_dark_96.save("assets/icons/ic_app.png")

    print("--> Generating custom UI icons...")
    lib_icon = create_3books_icon(96, dark_mode=False)
    lib_icon.save("assets/icons/ic_library.png")

    lib_icon_dark = create_3books_icon(96, dark_mode=True)
    lib_icon_dark.save("assets/icons/ic_library_dark.png")

    mode_light = create_mode_light_icon(96)
    mode_light.save("assets/icons/ic_mode_light.png")

    mode_dark = create_mode_dark_icon(96)
    mode_dark.save("assets/icons/ic_mode_dark.png")

    mode_sepia = create_mode_sepia_icon(96)
    mode_sepia.save("assets/icons/ic_mode_sepia.png")

    print("--> Generating splash screens for BerryLeaf...")
    targets = [
        ("splash-720x720.png", 720, 720, "Classic / Q10 / Q5"),
        ("splash-1440x1440.png", 1440, 1440, "Passport"),
        ("splash-720x1280.png", 720, 1280, "Z30 / Leap / Z3"),
        ("splash-768x1280.png", 768, 1280, "Z10 / P'9982"),
    ]
    for filename, w, h, device in targets:
        print(f"    Generating {filename} ({w}x{h} - {device})...")
        splash = create_splash(w, h)
        splash.save(filename)

    print("=== Brand generation complete! ===")

if __name__ == "__main__":
    main()
