#!/usr/bin/env python3
"""
Libris True BlackBerry 10 UI Preview & Device Mockup Generator
Produces authentic BlackBerry 10 Cascades screenshots and physical device mockups:
1. True 720x1280 (Z10/Z30 full-touch) and 720x720 (Classic/Q10/Passport square) native resolutions.
2. Authentic BlackBerry 10 System Status Bar (Signal bars, Viettel 4G/BlackBerry, Clock, Wi-Fi, Battery glyph).
3. Authentic Cascades Action Bar (with signature bottom-left Tab Drawer '≡' button and bottom-right Overflow '⋮' button).
4. Physical BlackBerry Device Mockups:
   - BlackBerry Classic (Q20) with optical trackpad toolbelt and 4-row sculpted QWERTY keyboard with silver frets.
   - BlackBerry Passport with wide stainless steel chassis and 3-row physical keyboard.
"""

import math
import os
import shutil
from PIL import Image, ImageDraw, ImageFont, ImageFilter

FONT_BOLD = "/usr/share/fonts/open-sans/OpenSans-Bold.ttf"
FONT_REG = "/usr/share/fonts/open-sans/OpenSans-Regular.ttf"
FONT_SEMI = "/usr/share/fonts/open-sans/OpenSans-Semibold.ttf"
FONT_SERIF = "/usr/share/fonts/google-noto/NotoSerif-Regular.ttf"
FONT_SERIF_BOLD = "/usr/share/fonts/google-noto/NotoSerif-Bold.ttf"

def get_text_size(draw, text, font):
    bbox = font.getbbox(text)
    return (bbox[2] - bbox[0], bbox[3] - bbox[1])

def draw_centered_text(draw, xy_center, text, font, fill):
    draw.text(xy_center, text, font=font, fill=fill, anchor="mm")

def paste_icon(img, icon_path, x, y, size=None, tint_color=None):
    if not os.path.exists(icon_path):
        return
    icon = Image.open(icon_path).convert("RGBA")
    if size:
        icon = icon.resize(size, Image.Resampling.LANCZOS)
    if tint_color:
        r, g, b, a = icon.split()
        tint = Image.new("RGBA", icon.size, tint_color)
        icon = Image.composite(tint, Image.new("RGBA", icon.size, (0, 0, 0, 0)), a)
    img.paste(icon, (int(x), int(y)), icon)

# ==============================================================================
# AUTHENTIC BLACKBERRY 10 CHROME COMPONENTS
# ==============================================================================

def draw_bb10_status_bar(img, draw, w, h_bar=38, bg=(8, 9, 12), text_color=(235, 238, 245)):
    """Draws authentic BlackBerry 10 Status Bar."""
    draw.rectangle([0, 0, w, h_bar], fill=bg)
    draw.line([(0, h_bar), (w, h_bar)], fill=(32, 35, 42), width=1)
    
    f_stat = ImageFont.truetype(FONT_BOLD, 12)
    f_clock = ImageFont.truetype(FONT_SEMI, 13)
    f_sm = ImageFont.truetype(FONT_REG, 11)

    # Signal bars (5 bars on left)
    sx = 16
    sy = 27
    for i in range(5):
        bh = 4 + i * 3
        fill = text_color if i < 4 else (90, 95, 105)
        draw.rectangle([sx + i * 4, sy - bh, sx + i * 4 + 2, sy], fill=fill)
    
    draw.text((sx + 26, 12), "Viettel 4G", font=f_stat, fill=text_color)
    
    # Centered Clock
    draw.text((w // 2, 12), "12:45", font=f_clock, fill=text_color, anchor="mt")
    
    # Right side: Wi-Fi arc, Battery level and percentage
    bx = w - 65
    draw.text((bx - 44, 12), "Wi-Fi", font=f_sm, fill=(195, 202, 215))
    draw.text((bx - 10, 12), "88%", font=f_stat, fill=text_color)
    
    # Battery icon outline + green level
    draw.rounded_rectangle([bx + 20, 13, bx + 45, 25], radius=2, outline=text_color, width=1)
    draw.rectangle([bx + 46, 16, bx + 48, 22], fill=text_color) # tip
    draw.rectangle([bx + 22, 15, bx + 42, 23], fill=(16, 185, 129)) # 88% green fill

def draw_cascades_titlebar(img, draw, w, title_text, y0=38, bar_h=64, left_action=None, right_action=None, bg_color=(18, 18, 18), text_color=(255, 255, 255)):
    """Draws native BlackBerry 10 Cascades TitleBar with gradient and dividers."""
    y1 = y0 + bar_h
    # Cascades gunmetal subtle gradient
    for y in range(y0, y1):
        ratio = (y - y0) / float(bar_h)
        r = int(bg_color[0] * (1.0 - 0.15 * ratio))
        g = int(bg_color[1] * (1.0 - 0.15 * ratio))
        b = int(bg_color[2] * (1.0 - 0.15 * ratio))
        draw.line([(0, y), (w, y)], fill=(r, g, b))
    
    draw.line([(0, y1), (w, y1)], fill=(40, 44, 54), width=1)
    
    tx = 20
    if left_action:
        if left_action.get("icon"):
            paste_icon(img, left_action["icon"], 16, y0 + (bar_h - 28) // 2, (28, 28))
            tx = 58
        elif left_action.get("text"):
            f_act = ImageFont.truetype(FONT_BOLD, 15)
            draw.text((16, y0 + 20), left_action["text"], font=f_act, fill=(16, 185, 129))
            tw, _ = get_text_size(draw, left_action["text"], f_act)
            tx = 30 + tw

    f_title = ImageFont.truetype(FONT_BOLD, 20)
    draw.text((tx, y0 + 18), title_text, font=f_title, fill=text_color)

    if right_action:
        if right_action.get("icon"):
            paste_icon(img, right_action["icon"], w - 48, y0 + (bar_h - 28) // 2, (28, 28))
        elif right_action.get("text"):
            f_act = ImageFont.truetype(FONT_BOLD, 15)
            tw, _ = get_text_size(draw, right_action["text"], f_act)
            draw.text((w - 20 - tw, y0 + 20), right_action["text"], font=f_act, fill=(16, 185, 129))

def draw_cascades_action_bar(img, draw, w, h, actions, bar_h=100, active_idx=None, has_tabs_drawer=True, has_overflow=True, bg_color=(12, 12, 12)):
    """
    Draws authentic BlackBerry 10 Cascades Action Bar.
    When showTabsOnActionBar is false, BB10 framework automatically anchors:
    - Leftmost: Sidebar Drawer button ('≡', label 'Mục')
    - Center slots: Active page actions (OnBar)
    - Rightmost: Cascades Overflow button ('⋮', label 'Thêm')
    """
    y1 = h - bar_h
    # Dark Cascades action bar background
    draw.rectangle([0, y1, w, h], fill=bg_color)
    draw.line([(0, y1), (w, y1)], fill=(40, 44, 56), width=1)

    total_items = (1 if has_tabs_drawer else 0) + len(actions) + (1 if has_overflow else 0)
    item_w = w / float(total_items)
    f_lbl = ImageFont.truetype(FONT_SEMI, 11)

    slot_idx = 0

    # 1. Signature BB10 Tabs Drawer button (≡)
    if has_tabs_drawer:
        cx = int(slot_idx * item_w + item_w / 2)
        cy = y1 + 30
        lw = 24
        # 3 horizontal lines of drawer
        draw.rectangle([cx - lw//2, cy - 8, cx + lw//2, cy - 6], fill=(210, 215, 225))
        draw.rectangle([cx - lw//2, cy - 1, cx + lw//2, cy + 1], fill=(210, 215, 225))
        draw.rectangle([cx - lw//2, cy + 6, cx + lw//2, cy + 8], fill=(210, 215, 225))
        draw_centered_text(draw, (cx, y1 + 72), "Mục", f_lbl, (160, 165, 175))
        slot_idx += 1

    # 2. Middle Page Actions
    for i, act in enumerate(actions):
        cx = int(slot_idx * item_w + item_w / 2)
        is_active = (i == active_idx)
        color = (16, 185, 129) if is_active else (210, 215, 225)
        lbl_color = (16, 185, 129) if is_active else (160, 165, 175)

        if act.get("icon") and os.path.exists(act["icon"]):
            paste_icon(img, act["icon"], cx - 14, y1 + 16, (28, 28), tint_color=color if is_active else None)
        else:
            draw.ellipse([cx - 12, y1 + 18, cx + 12, y1 + 42], outline=color, width=2)

        draw_centered_text(draw, (cx, y1 + 72), act.get("title", ""), f_lbl, lbl_color)

        if is_active:
            draw.rectangle([cx - 24, h - 3, cx + 24, h], fill=(16, 185, 129))

        slot_idx += 1

    # 3. Signature BB10 Overflow button (⋮)
    if has_overflow:
        cx = int(slot_idx * item_w + item_w / 2)
        cy = y1 + 30
        for dy in [-8, 0, 8]:
            draw.ellipse([cx - 2, cy + dy - 2, cx + 2, cy + dy + 2], fill=(210, 215, 225))
        draw_centered_text(draw, (cx, y1 + 72), "Thêm", f_lbl, (160, 165, 175))

def draw_book_cover(draw, x, y, w, h, title, author, base_hue="emerald", progress=None):
    """Draws a tactile book cover thumbnail with real spine line and typography."""
    color_map = {
        "emerald": ((6, 78, 59), (16, 185, 129), (4, 47, 36)),
        "blue": ((30, 58, 138), (59, 130, 246), (15, 23, 42)),
        "amber": ((120, 53, 15), (245, 158, 11), (69, 26, 3)),
        "purple": ((88, 28, 135), (168, 85, 247), (59, 7, 100)),
        "slate": ((34, 34, 34), (100, 116, 139), (15, 23, 42)),
        "red": ((127, 29, 29), (239, 68, 68), (69, 10, 10))
    }
    bg, spine, dark = color_map.get(base_hue, color_map["emerald"])

    # Cover shadow and rounded base
    draw.rounded_rectangle([x + 2, y + 2, x + w + 2, y + h + 2], radius=6, fill=(0, 0, 0))
    draw.rounded_rectangle([x, y, x + w, y + h], radius=6, fill=bg, outline=(60, 64, 76), width=1)

    # Spine line
    draw.rectangle([x + 6, y + 4, x + 12, y + h - 4], fill=spine)
    draw.line([(x + 13, y + 4), (x + 13, y + h - 4)], fill=dark, width=1)

    f_t = ImageFont.truetype(FONT_BOLD, max(10, int(w * 0.095)))
    f_a = ImageFont.truetype(FONT_REG, max(8, int(w * 0.075)))

    words = title.split()
    lines = []
    cur = ""
    for wd in words:
        if len(cur + " " + wd) < 13:
            cur += " " + wd if cur else wd
        else:
            lines.append(cur)
            cur = wd
    if cur: lines.append(cur)

    ty = y + int(h * 0.22)
    for line in lines[:3]:
        draw.text((x + 18, ty), line, font=f_t, fill=(255, 255, 255))
        ty += int(w * 0.13)

    draw.text((x + 18, y + h - int(w * 0.18)), author, font=f_a, fill=(209, 213, 219))

    if progress is not None:
        draw.rectangle([x, y + h - 4, x + w, y + h], fill=(20, 20, 24))
        pw = int(w * (progress / 100.0))
        draw.rectangle([x, y + h - 4, x + pw, y + h], fill=(16, 185, 129))

# ==============================================================================
# SCREEN 1: HOME / RECENT PAGE (720x1280)
# ==============================================================================
def render_recent_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Cascades TitleBar with Search Action on the Right
    draw_cascades_titlebar(img, draw, w, "Libris", y0=38, right_action={"icon": "assets/icons/ic_search.png"})

    # 3. Main Content
    pad = 16
    y_start = 118

    # Continue Reading Hero Card
    card_h = 240
    draw.rounded_rectangle([pad, y_start, w - pad, y_start + card_h], radius=10, fill=(17, 17, 17), outline=(34, 34, 34), width=1)

    # Cover inside card
    cov_w, cov_h = 135, 195
    cov_x = pad + 18
    cov_y = y_start + (card_h - cov_h) // 2
    draw_book_cover(draw, cov_x, cov_y, cov_w, cov_h, "Nhà Giả Kim", "Paulo Coelho", "emerald", 70)

    # Text metadata
    tx = cov_x + cov_w + 18
    ty = cov_y + 4

    # Format Badge on right
    draw.rounded_rectangle([w - pad - 68, ty, w - pad - 18, ty + 24], radius=4, fill=(34, 34, 34))
    draw_centered_text(draw, (w - pad - 43, ty + 12), "EPUB", ImageFont.truetype(FONT_BOLD, 10), (203, 213, 225))

    f_title = ImageFont.truetype(FONT_BOLD, 21)
    draw.text((tx, ty), "Nhà Giả Kim", font=f_title, fill=(255, 255, 255))

    f_auth = ImageFont.truetype(FONT_REG, 15)
    draw.text((tx, ty + 30), "Paulo Coelho", font=f_auth, fill=(150, 150, 150))

    # Progress bar
    pb_y = ty + 72
    pb_w = w - pad - tx - 24
    draw.rounded_rectangle([tx, pb_y, tx + pb_w, pb_y + 8], radius=4, fill=(34, 34, 34))
    draw.rounded_rectangle([tx, pb_y, tx + int(pb_w * 0.70), pb_y + 8], radius=4, fill=(16, 185, 129))

    f_pct = ImageFont.truetype(FONT_BOLD, 13)
    draw.text((tx, pb_y + 16), "Tiến độ: 70%", font=f_pct, fill=(16, 185, 129))

    # Button 'Đọc tiếp'
    btn_y = ty + 130
    btn_w = pb_w
    btn_h = 46
    draw.rounded_rectangle([tx, btn_y, tx + btn_w, btn_y + btn_h], radius=6, fill=(16, 185, 129))
    paste_icon(img, "assets/icons/ic_reading.png", tx + 20, btn_y + 11, (24, 24), tint_color=(255, 255, 255, 255))
    draw.text((tx + 56, btn_y + 13), "Đọc tiếp", font=ImageFont.truetype(FONT_BOLD, 15), fill=(255, 255, 255))

    # Header: SÁCH GẦN ĐÂY
    hy = y_start + card_h + 20
    draw.text((pad, hy), "SÁCH GẦN ĐÂY", font=ImageFont.truetype(FONT_BOLD, 14), fill=(150, 150, 150))

    # List of recent items
    recent_list = [
        {"title": "Hoàng Tử Bé", "author": "Antoine de Saint-Exupéry", "fmt": "EPUB", "pct": 45, "hue": "blue"},
        {"title": "Sapiens: Lược Sử Loài Người", "author": "Yuval Noah Harari", "fmt": "EPUB", "pct": 18, "hue": "amber"},
        {"title": "Cây Cam Ngọt Của Tôi", "author": "José Mauro de Vasconcelos", "fmt": "EPUB", "pct": 92, "hue": "purple"},
        {"title": "Machine Learning Cơ Bản", "author": "Vũ Hữu Tiệp", "fmt": "PDF", "pct": 55, "hue": "red"},
        {"title": "Bắt Trẻ Đồng Xanh", "author": "J. D. Salinger", "fmt": "TXT", "pct": 30, "hue": "slate"}
    ]

    ly = hy + 26
    row_h = 100
    for i, it in enumerate(recent_list):
        if ly + row_h > h - 110: break
        draw.rounded_rectangle([pad, ly, w - pad, ly + row_h - 10], radius=8, fill=(18, 22, 30), outline=(35, 40, 52), width=1)
        
        # Mini cover
        mc_w, mc_h = 56, 76
        draw_book_cover(draw, pad + 12, ly + 7, mc_w, mc_h, it["title"], it["author"], it["hue"], it["pct"])
        
        # Text
        rx = pad + 12 + mc_w + 14
        draw.text((rx, ly + 14), it["title"], font=ImageFont.truetype(FONT_BOLD, 16), fill=(245, 247, 250))
        draw.text((rx, ly + 38), it["author"] + " • " + it["fmt"], font=ImageFont.truetype(FONT_REG, 13), fill=(150, 150, 150))
        
        # Pct badge
        draw.text((w - pad - 60, ly + 28), f"{it['pct']}%", font=ImageFont.truetype(FONT_BOLD, 14), fill=(16, 185, 129))
        
        ly += row_h

    # 4. Cascades Action Bar
    actions = [
        {"title": "Mở Tệp", "icon": "assets/icons/ic_open.png"},
        {"title": "Thư Viện", "icon": "assets/icons/ic_library.png"},
        {"title": "Tiến Độ", "icon": "assets/icons/ic_chart.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, actions, active_idx=None)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 2: BOOKSHELF POPULATED GRID (720x1280)
# ==============================================================================
def render_library_populated_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Cascades TitleBar
    draw_cascades_titlebar(img, draw, w, "Tủ Sách", y0=38, right_action={"icon": "assets/icons/ic_search.png"})

    # 3. Single-Row Bookshelf Sub-header Toolbar
    pad = 16
    sy1 = 108
    draw.text((pad, sy1 + 14), "Tủ Sách • 6 sách", font=ImageFont.truetype(FONT_BOLD, 19), fill=(226, 232, 240))
    
    # View mode switcher button (Grid/List)
    draw.rounded_rectangle([w - pad - 84, sy1 - 4, w - pad, sy1 + 52], radius=8, fill=(30, 38, 52))
    paste_icon(img, "assets/icons/ic_grid.png", w - pad - 62, sy1 + 5, (40, 40))

    # 4. Grid of Books (3 columns) directly below toolbar!
    grid_y = 176
    cols = 3
    cw = (w - pad * 2 - (cols - 1) * 16) // cols
    ch_cover = int(cw * 1.45)

    books = [
        {"title": "Nhà Giả Kim", "author": "Paulo Coelho", "hue": "emerald", "pct": 70},
        {"title": "Hoàng Tử Bé", "author": "Saint-Exupéry", "hue": "blue", "pct": 45},
        {"title": "Sapiens", "author": "Yuval Harari", "hue": "amber", "pct": 18},
        {"title": "Cây Cam Ngọt", "author": "Vasconcelos", "hue": "purple", "pct": 92},
        {"title": "Bắt Trẻ Đồng Xanh", "author": "J.D. Salinger", "hue": "slate", "pct": 30},
        {"title": "Machine Learning", "author": "Vũ Hữu Tiệp", "hue": "red", "pct": 55}
    ]

    f_title = ImageFont.truetype(FONT_BOLD, 13)
    f_auth = ImageFont.truetype(FONT_REG, 11)

    for i, b in enumerate(books):
        col = i % cols
        row = i // cols
        bx = pad + col * (cw + 16)
        by = grid_y + row * (ch_cover + 78)

        draw_book_cover(draw, bx, by, cw, ch_cover, b["title"], b["author"], b["hue"], b["pct"])
        draw.text((bx + 2, by + ch_cover + 8), b["title"], font=f_title, fill=(243, 244, 246))
        draw.text((bx + 2, by + ch_cover + 28), b["author"], font=f_auth, fill=(156, 163, 175))

    # 5. Cascades Action Bar
    actions = [
        {"title": "Mở Tệp", "icon": "assets/icons/ic_open.png"},
        {"title": "Kho OPDS", "icon": "assets/icons/ic_download.png"},
        {"title": "Bộ Lọc", "icon": "assets/icons/ic_filter.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, actions, active_idx=None)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 3: BOOKSHELF EMPTY STATE (720x1280)
# ==============================================================================
def render_library_empty_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Cascades TitleBar
    draw_cascades_titlebar(img, draw, w, "Tủ Sách", y0=38, right_action={"icon": "assets/icons/ic_search.png"})

    # 3. Sub-header Toolbar
    pad = 16
    sy1 = 108
    draw.text((pad, sy1 + 14), "Tủ Sách • 0", font=ImageFont.truetype(FONT_BOLD, 19), fill=(226, 232, 240))
    draw.rounded_rectangle([w - pad - 84, sy1 - 4, w - pad, sy1 + 52], radius=8, fill=(30, 38, 52))
    paste_icon(img, "assets/icons/ic_grid.png", w - pad - 62, sy1 + 5, (40, 40))

    # 4. Center Empty State
    mid_y = 560
    
    # Large Faded Book Icon (35% opacity)
    ic_path = "assets/icons/ic_book.png"
    if os.path.exists(ic_path):
        orig_ic = Image.open(ic_path).convert("RGBA").resize((240, 240), Image.Resampling.LANCZOS)
        r, g, b, a = orig_ic.split()
        a = a.point(lambda p: int(p * 0.35))
        faded_ic = Image.merge("RGBA", (r, g, b, a))
        img.paste(faded_ic, (w // 2 - 120, mid_y - 160), faded_ic)

    f_empt = ImageFont.truetype(FONT_BOLD, 22)
    draw_centered_text(draw, (w // 2, mid_y + 110), "Tủ sách đang trống", f_empt, (243, 244, 246))

    f_sub = ImageFont.truetype(FONT_REG, 15)
    draw_centered_text(draw, (w // 2, mid_y + 146), "Tủ sách của bạn đang trống.", f_sub, (150, 150, 150))
    draw_centered_text(draw, (w // 2, mid_y + 172), "Hãy mở tệp mới hoặc duyệt thư mục để bắt đầu đọc sách!", f_sub, (150, 150, 150))

    # 5. Cascades Action Bar
    actions = [
        {"title": "Mở Tệp", "icon": "assets/icons/ic_open.png"},
        {"title": "Kho OPDS", "icon": "assets/icons/ic_download.png"},
        {"title": "Bộ Lọc", "icon": "assets/icons/ic_filter.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, actions)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 4: IMMERSIVE READER PAGE (720x1280)
# ==============================================================================
def render_reader_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (244, 236, 216)) # Warm Sepia #F4ECD8
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar (Dark sepia tint)
    draw_bb10_status_bar(img, draw, w, bg=(42, 35, 28), text_color=(235, 225, 210))

    # 2. Reader Top Bar with Wide Back Button, Divider & Centered Title
    ty0 = 38
    tb_h = 60
    draw.rectangle([0, ty0, w, ty0 + tb_h], fill=(24, 26, 32)) # Dark BB10 TitleBar
    draw.line([(0, ty0 + tb_h), (w, ty0 + tb_h)], fill=(0, 168, 223), width=2) # BB10 Cyan Accent line

    # Wide back button
    btn_w = 104
    paste_icon(img, "assets/icons/ic_previous.png", (btn_w - 26) // 2, ty0 + (tb_h - 26) // 2, (26, 26), tint_color=(255, 255, 255, 255))

    # Vertical divider line
    div_x = btn_w
    draw.line([(div_x, ty0 + 12), (div_x, ty0 + tb_h - 12)], fill=(255, 255, 255, 55), width=2)

    # Book Title centered between divider and right edge
    f_title = ImageFont.truetype(FONT_BOLD, 18)
    title_center_x = (div_x + w) // 2
    draw_centered_text(draw, (title_center_x, ty0 + tb_h // 2), "Harry Potter Series Book 1-7", f_title, (255, 255, 255))

    # 3. Typography Reading Viewport
    pad = 38
    vy = ty0 + tb_h + 30

    f_ch = ImageFont.truetype(FONT_SERIF_BOLD, 22)
    draw.text((pad, vy), "Chương Một: Giấc Mơ Của Cậu Bé Chăn Cừu", font=f_ch, fill=(45, 30, 15))

    f_body = ImageFont.truetype(FONT_SERIF, 17)
    paragraphs = [
        "Cậu bé tên là Santiago. Trời vừa chập tối khi cậu cùng đàn cừu đến một nhà thờ cổ hoang phế. Mái nhà thờ đã sụp từ lâu và một cây tiêu thụ lớn mọc lên ngay nơi trước kia là phòng thánh.",
        "Cậu quyết định qua đêm tại đây. Cậu lùa đàn cừu qua khung cửa đổ nát, rồi dùng vài thanh gỗ chặn lối lại để đêm đến cừu không đi lạc. Nơi này chẳng hề có chó sói, nhưng cậu biết hễ có một con cừu nào sút chuồng thì hôm sau cậu sẽ mất cả ngày để đi tìm.",
        "Cậu trải áo khoác lên sàn gạch phủ đầy bụi và dùng cuốn sách đang đọc dở làm gối. Cậu nhủ thầm phải tìm đọc những cuốn sách dày hơn: chúng vừa đọc được lâu hơn lại vừa là một chiếc gối êm hơn nhiều.",
        "Khi cậu thức giấc thì trời còn mờ tối. Nhìn lên trên, cậu thấy những vì sao lấp lánh qua mái trần thủng. Cậu muốn ngủ thêm một lát nữa, nhưng giấc mơ đêm trước lại hiện về..."
    ]

    cur_y = vy + 46
    for p in paragraphs:
        words = p.split()
        lines = []
        cur = ""
        for wd in words:
            if len(cur + " " + wd) < 42:
                cur += " " + wd if cur else wd
            else:
                lines.append(cur)
                cur = wd
        if cur: lines.append(cur)

        for line in lines:
            draw.text((pad, cur_y), line, font=f_body, fill=(55, 40, 25))
            cur_y += 32
        cur_y += 20

    # 4. Reader Dedicated Bottom Status Bar
    stat_y = h - 146
    stat_h = 46
    draw.rectangle([0, stat_y, w, stat_y + stat_h], fill=(230, 220, 198))
    draw.line([(0, stat_y), (w, stat_y)], fill=(210, 200, 178), width=1)

    f_stat = ImageFont.truetype(FONT_REG, 13)
    draw.text((pad, stat_y + 14), "Phần 1: Khởi hành đến Tarifa", font=f_stat, fill=(90, 75, 60))
    draw.text((w - pad - 210, stat_y + 14), "Trang 42 / 215 • 70%", font=ImageFont.truetype(FONT_BOLD, 13), fill=(40, 30, 20))

    # 5. Cascades Bottom Action Bar (Sepia Mode)
    reader_actions = [
        {"title": "Mục lục", "icon": "assets/icons/ic_toc.png"},
        {"title": "Tìm kiếm", "icon": "assets/icons/ic_search.png"},
        {"title": "Sepia", "icon": "assets/icons/ic_mode_sepia.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, reader_actions, bg_color=(42, 35, 28), active_idx=2, has_tabs_drawer=False, has_overflow=True)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 5: TABLE OF CONTENTS (720x1280)
# ==============================================================================
def render_toc_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Sheet TitleBar
    draw_cascades_titlebar(img, draw, w, "Mục Lục — Nhà Giả Kim", y0=38, left_action={"text": "Đóng"})

    # 3. Chapter List
    chapters = [
        ("Lời Tựa: Người Tìm Vàng", 1, False),
        ("Phần Một: Cuộc Gặp Gỡ Ở Tarifa", 5, False),
        ("Chương 1: Giấc Mơ Của Cậu Bé Chăn Cừu", 14, True), # Active
        ("Chương 2: Vị Vua Xứ Salem & Đá Urim, Thummim", 38, False),
        ("Chương 3: Cửa Hàng Pha Lê Trên Đồi", 64, False),
        ("Phần Hai: Đoàn Lữ Hành Băng Sa Mạc", 92, False),
        ("Chương 4: Ốc Đảo Al-Fayoum", 120, False),
        ("Chương 5: Gặp Gỡ Nhà Giả Kim Đích Thực", 156, False),
        ("Chương 6: Lời Nguyện Cầu Của Gió & Mặt Trời", 188, False),
        ("Phần Kết: Kho Báu Dưới Cội Cây Tiêu Thụ", 212, False)
    ]

    pad = 20
    ly = 120
    row_h = 74
    f_num = ImageFont.truetype(FONT_BOLD, 15)
    f_t = ImageFont.truetype(FONT_BOLD, 16)
    f_pg = ImageFont.truetype(FONT_REG, 13)

    for i, (title, page, is_active) in enumerate(chapters):
        if is_active:
            draw.rounded_rectangle([pad, ly, w - pad, ly + row_h - 8], radius=8, fill=(16, 185, 129))
            draw.text((pad + 18, ly + 22), str(i + 1), font=f_num, fill=(255, 255, 255))
            draw.text((pad + 60, ly + 22), title, font=f_t, fill=(255, 255, 255))
            draw.text((w - pad - 68, ly + 22), f"Tr. {page}", font=f_pg, fill=(240, 253, 244))
        else:
            draw.rounded_rectangle([pad, ly, w - pad, ly + row_h - 8], radius=8, fill=(18, 22, 30), outline=(32, 36, 48), width=1)
            draw.text((pad + 18, ly + 22), str(i + 1), font=f_num, fill=(150, 150, 150))
            draw.text((pad + 60, ly + 22), title, font=f_t, fill=(226, 232, 240))
            draw.text((w - pad - 68, ly + 22), f"Tr. {page}", font=f_pg, fill=(150, 150, 150))

        ly += row_h

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 6: SETTINGS SHEET (720x1280)
# ==============================================================================
def render_settings_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Sheet TitleBar
    draw_cascades_titlebar(img, draw, w, "Cài Đặt", y0=38, left_action={"text": "Đóng"})

    # 3. Settings Categories
    settings_items = [
        ("Hiển thị & Giao diện", "Chủ đề tối, màu nền đọc sách, phông chữ & cỡ chữ", "assets/icons/ic_theme.png"),
        ("Thao tác đọc", "Vuốt lật trang (Paged) hoặc cuộn liên tục (Scroll)", "assets/icons/ic_reading.png"),
        ("Chân trang (Footer)", "Hiển thị tên chương, tiến độ, pin & đồng hồ", "assets/icons/ic_chart.png"),
        ("Đọc sách giọng nói (TTS)", "Bộ máy đọc AI, giọng đọc và tốc độ đọc", "assets/icons/ic_tts.png"),
        ("Tủ sách & Quản lý", "Bố cục lưới/danh sách, quét tệp & sao lưu bảo toàn", "assets/icons/ic_library.png"),
        ("Thông tin ứng dụng", "Giới thiệu Libris 2.0, phím tắt & nhật ký cập nhật", "assets/icons/ic_info.png")
    ]

    pad = 20
    ly = 120
    row_h = 100

    for title, desc, icon in settings_items:
        draw.rounded_rectangle([pad, ly, w - pad, ly + row_h - 10], radius=8, fill=(18, 22, 30), outline=(35, 40, 52), width=1)
        
        # Icon
        paste_icon(img, icon, pad + 18, ly + 24, (36, 36))
        
        # Text
        tx = pad + 18 + 48
        draw.text((tx, ly + 20), title, font=ImageFont.truetype(FONT_BOLD, 17), fill=(245, 247, 250))
        draw.text((tx, ly + 48), desc, font=ImageFont.truetype(FONT_REG, 13), fill=(150, 150, 150))

        # Chevron ›
        draw.text((w - pad - 28, ly + 32), "›", font=ImageFont.truetype(FONT_BOLD, 22), fill=(100, 110, 130))

        ly += row_h

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 7: SQUARE 1:1 BOOKSHELF (720x720 - Q10 / Classic / Passport)
# ==============================================================================
def render_square_library_page(out_path):
    w, h = 720, 720
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar (32px for square screen)
    draw_bb10_status_bar(img, draw, w, h_bar=32)

    # 2. TitleBar (52px)
    draw_cascades_titlebar(img, draw, w, "Tủ Sách", y0=32, bar_h=52, right_action={"icon": "assets/icons/ic_search.png"})

    # 3. Toolbar
    pad = 16
    sy1 = 92
    draw.text((pad, sy1 + 10), "Tủ Sách • 6 sách", font=ImageFont.truetype(FONT_BOLD, 18), fill=(226, 232, 240))
    draw.rounded_rectangle([w - pad - 76, sy1 - 4, w - pad, sy1 + 46], radius=8, fill=(30, 38, 52))
    paste_icon(img, "assets/icons/ic_grid.png", w - pad - 56, sy1 + 3, (36, 36))

    # 4. Grid of Books (3 columns)
    grid_y = 144
    cols = 3
    cw = (w - pad * 2 - (cols - 1) * 16) // cols
    ch_cover = int(cw * 1.35)

    books = [
        {"title": "Nhà Giả Kim", "author": "Paulo Coelho", "hue": "emerald", "pct": 70},
        {"title": "Hoàng Tử Bé", "author": "Saint-Exupéry", "hue": "blue", "pct": 45},
        {"title": "Sapiens", "author": "Yuval Harari", "hue": "amber", "pct": 18}
    ]

    for i, b in enumerate(books):
        bx = pad + i * (cw + 16)
        by = grid_y
        draw_book_cover(draw, bx, by, cw, ch_cover, b["title"], b["author"], b["hue"], b["pct"])
        draw.text((bx + 2, by + ch_cover + 8), b["title"], font=ImageFont.truetype(FONT_BOLD, 13), fill=(243, 244, 246))
        draw.text((bx + 2, by + ch_cover + 26), b["author"], font=ImageFont.truetype(FONT_REG, 11), fill=(156, 163, 175))

    # 5. Cascades Action Bar (88px for square screen)
    actions = [
        {"title": "Mở Tệp", "icon": "assets/icons/ic_open.png"},
        {"title": "Kho OPDS", "icon": "assets/icons/ic_download.png"},
        {"title": "Bộ Lọc", "icon": "assets/icons/ic_filter.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, actions, bar_h=88)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 8: SQUARE 1:1 READER PAGE (720x720 - Q10 / Classic / Passport)
# ==============================================================================
def render_square_reader_page(out_path):
    w, h = 720, 720
    img = Image.new("RGB", (w, h), (244, 236, 216)) # Sepia #F4ECD8
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w, h_bar=32, bg=(42, 35, 28), text_color=(235, 225, 210))

    # 2. Reader Top Bar with Wide Back Button, Divider & Centered Title
    ty0 = 32
    tb_h = 52
    draw.rectangle([0, ty0, w, ty0 + tb_h], fill=(24, 26, 32))
    draw.line([(0, ty0 + tb_h), (w, ty0 + tb_h)], fill=(0, 168, 223), width=2)
    btn_w = 96
    paste_icon(img, "assets/icons/ic_previous.png", (btn_w - 24) // 2, ty0 + (tb_h - 24) // 2, (24, 24), tint_color=(255, 255, 255, 255))
    div_x = btn_w
    draw.line([(div_x, ty0 + 10), (div_x, ty0 + tb_h - 10)], fill=(255, 255, 255, 55), width=2)
    f_title = ImageFont.truetype(FONT_BOLD, 17)
    title_center_x = (div_x + w) // 2
    draw_centered_text(draw, (title_center_x, ty0 + tb_h // 2), "Harry Potter Series Book 1-7", f_title, (255, 255, 255))

    # 3. Reading Viewport
    pad = 32
    vy = ty0 + tb_h + 20
    draw.text((pad, vy), "Chương Một: Giấc Mơ Cậu Bé Chăn Cừu", font=ImageFont.truetype(FONT_SERIF_BOLD, 19), fill=(45, 30, 15))

    f_body = ImageFont.truetype(FONT_SERIF, 16)
    paragraphs = [
        "Cậu bé tên là Santiago. Trời vừa chập tối khi cậu cùng đàn cừu đến một nhà thờ cổ hoang phế. Mái nhà thờ đã sụp từ lâu và một cây tiêu thụ lớn mọc lên ngay nơi trước kia là phòng thánh.",
        "Cậu quyết định qua đêm tại đây. Cậu lùa đàn cừu qua khung cửa đổ nát, rồi dùng vài thanh gỗ chặn lối lại để đêm đến cừu không đi lạc. Nơi này chẳng hề có chó sói, nhưng cậu biết hễ có một con cừu nào sút chuồng thì hôm sau cậu sẽ mất cả ngày để đi tìm...",
        "Cậu trải áo khoác lên sàn gạch phủ đầy bụi và dùng cuốn sách đang đọc dở làm gối..."
    ]

    cur_y = vy + 36
    for p in paragraphs:
        words = p.split()
        lines = []
        cur = ""
        for wd in words:
            if len(cur + " " + wd) < 46:
                cur += " " + wd if cur else wd
            else:
                lines.append(cur)
                cur = wd
        if cur: lines.append(cur)

        for line in lines:
            if cur_y + 28 > h - 130: break
            draw.text((pad, cur_y), line, font=f_body, fill=(55, 40, 25))
            cur_y += 28
        cur_y += 14

    # 4. Reader Dedicated Status Bar
    stat_y = h - 128
    stat_h = 40
    draw.rectangle([0, stat_y, w, stat_y + stat_h], fill=(230, 220, 198))
    draw.line([(0, stat_y), (w, stat_y)], fill=(210, 200, 178), width=1)
    draw.text((pad, stat_y + 11), "Phần 1: Tarifa", font=ImageFont.truetype(FONT_REG, 12), fill=(90, 75, 60))
    draw.text((w - pad - 170, stat_y + 11), "Tr. 42 / 215 • 70%", font=ImageFont.truetype(FONT_BOLD, 12), fill=(40, 30, 20))

    # 5. Bottom Action Bar
    reader_actions = [
        {"title": "Mục lục", "icon": "assets/icons/ic_toc.png"},
        {"title": "Tìm kiếm", "icon": "assets/icons/ic_search.png"},
        {"title": "Sepia", "icon": "assets/icons/ic_mode_sepia.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, reader_actions, bar_h=88, bg_color=(42, 35, 28), active_idx=2, has_tabs_drawer=False, has_overflow=True)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# HARDWARE MOCKUP 1: BLACKBERRY CLASSIC Q20
# ==============================================================================
def render_blackberry_classic_mockup(screen_img_path, out_path):
    """
    Renders the app on an authentic BlackBerry Classic Q20 hardware mockup:
    - Top bezel: Ear speaker mesh, front camera, red LED notification, silver BlackBerry logo
    - Screen: Exact 720x720 square display
    - Toolbelt: Physical Call, Menu, Optical Trackpad with silver ring, Back, End Call
    - Keyboard: 4-row sculpted QWERTY keys separated by silver stainless steel frets
    """
    screen_img = Image.open(screen_img_path).convert("RGB").resize((720, 720), Image.Resampling.LANCZOS)
    sw, sh = 720, 720
    
    dw = 840
    dh = 1380
    mockup = Image.new("RGB", (dw, dh), (14, 15, 18))
    draw = ImageDraw.Draw(mockup)

    # 1. Outer Chassis with rounded corners & metallic chamfered rim
    draw.rounded_rectangle([6, 6, dw - 6, dh - 6], radius=42, fill=(18, 20, 24), outline=(100, 105, 118), width=4)
    draw.rounded_rectangle([12, 12, dw - 12, dh - 12], radius=38, fill=(8, 9, 12), outline=(35, 38, 46), width=2)

    # 2. Forehead (Top Bezel)
    # Speaker grill
    sp_w, sp_h = 100, 9
    draw.rounded_rectangle([dw // 2 - sp_w // 2, 34, dw // 2 + sp_w // 2, 34 + sp_h], radius=4, fill=(45, 48, 56), outline=(85, 90, 105), width=1)
    
    # Front Camera lens
    draw.ellipse([dw // 2 - 110, 31, dw // 2 - 94, 47], fill=(20, 25, 38), outline=(60, 68, 85), width=2)
    draw.ellipse([dw // 2 - 104, 37, dw // 2 - 100, 41], fill=(80, 120, 200)) # Blue lens reflection
    
    # Red LED notification
    draw.ellipse([dw // 2 + 96, 32, dw // 2 + 110, 46], fill=(220, 38, 38))
    draw.ellipse([dw // 2 + 100, 36, dw // 2 + 106, 42], fill=(254, 202, 202)) # Glow core

    # Silver Metallic BlackBerry Logo
    f_logo = ImageFont.truetype(FONT_BOLD, 17)
    draw_centered_text(draw, (dw // 2, 70), "BlackBerry", f_logo, (215, 220, 230))

    # 3. Square Screen (720x720)
    sx = (dw - sw) // 2
    sy = 102
    mockup.paste(screen_img, (sx, sy))
    draw.rectangle([sx - 1, sy - 1, sx + sw, sy + sh], outline=(50, 55, 68), width=1)

    # 4. BlackBerry Classic Toolbelt
    ty = sy + sh + 12
    th = 58
    draw.rounded_rectangle([sx, ty, sx + sw, ty + th], radius=8, fill=(18, 18, 18), outline=(40, 40, 40), width=1)

    # Toolbelt buttons: Call (Green) | Menu | Trackpad | Back | End Call (Red)
    tb_w = sw / 5.0
    f_tb = ImageFont.truetype(FONT_BOLD, 16)
    
    f_tb_lbl = ImageFont.truetype(FONT_BOLD, 12)
    # Call key (Send - Green line & label)
    draw_centered_text(draw, (sx + tb_w * 0.5, ty + th // 2), "SEND", f_tb_lbl, (16, 185, 129))
    
    # BlackBerry Menu Key
    draw_centered_text(draw, (sx + tb_w * 1.5, ty + th // 2), "MENU", f_tb_lbl, (210, 215, 225))
    
    # Optical Trackpad in Center
    tpad_w, tpad_h = 56, 42
    tcx = sx + tb_w * 2.5
    draw.rounded_rectangle([tcx - tpad_w // 2, ty + (th - tpad_h) // 2, tcx + tpad_w // 2, ty + (th + tpad_h) // 2], radius=8, fill=(10, 11, 14), outline=(180, 185, 195), width=2)
    
    # Back Key
    draw_centered_text(draw, (sx + tb_w * 3.5, ty + th // 2), "BACK", f_tb_lbl, (210, 215, 225))
    
    # End Call Key (End - Red label)
    draw_centered_text(draw, (sx + tb_w * 4.5, ty + th // 2), "END", f_tb_lbl, (239, 68, 68))

    # 5. Iconic 4-Row Physical QWERTY Keyboard
    ky = ty + th + 10
    kh = 430
    row_h = kh // 4
    
    f_key = ImageFont.truetype(FONT_BOLD, 15)
    f_sym = ImageFont.truetype(FONT_REG, 10)

    rows = [
        ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
        ["A", "S", "D", "F", "G", "H", "J", "K", "L", "del"],
        ["alt", "Z", "X", "C", "V", "B", "N", "M", "$", "↵"],
        ["aA", "mic", " ", "0", "sym"]
    ]

    for r_idx, r_keys in enumerate(rows):
        ry = ky + r_idx * row_h
        # Stainless steel fret separating rows
        draw.line([(sx, ry), (sx + sw, ry)], fill=(175, 180, 192), width=3)
        draw.line([(sx, ry + 1), (sx + sw, ry + 1)], fill=(230, 235, 245), width=1)

        kw = sw / float(len(r_keys))
        for k_idx, k_char in enumerate(r_keys):
            kx = sx + k_idx * kw
            # Key body
            draw.rounded_rectangle([kx + 2, ry + 5, kx + kw - 2, ry + row_h - 4], radius=4, fill=(24, 27, 34), outline=(42, 46, 56), width=1)
            # Sculpted key shape reflection
            draw.line([(kx + 6, ry + 8), (kx + kw - 6, ry + 8)], fill=(65, 70, 85), width=1)
            # Text
            draw_centered_text(draw, (kx + kw / 2, ry + row_h / 2), k_char, f_key, (230, 235, 245))

    # Bottom fret
    draw.line([(sx, ky + kh), (sx + sw, ky + kh)], fill=(175, 180, 192), width=3)

    mockup.save(out_path, "PNG", quality=95)
    print(f"Saved Device Mockup: {out_path}")

# ==============================================================================
# HARDWARE MOCKUP 2: BLACKBERRY PASSPORT
# ==============================================================================
def render_blackberry_passport_mockup(screen_img_path, out_path):
    """
    Renders the app on an authentic BlackBerry Passport hardware mockup:
    - Extra-wide stainless steel chassis
    - 1:1 wide screen
    - 3-row touch-enabled physical keyboard
    """
    screen_img = Image.open(screen_img_path).convert("RGB").resize((760, 760), Image.Resampling.LANCZOS)
    sw, sh = 760, 760
    
    dw = 880
    dh = 1240
    mockup = Image.new("RGB", (dw, dh), (14, 15, 18))
    draw = ImageDraw.Draw(mockup)

    # 1. Stainless Steel Outer Frame
    draw.rounded_rectangle([4, 4, dw - 4, dh - 4], radius=18, fill=(18, 20, 24), outline=(160, 165, 178), width=5)
    draw.rounded_rectangle([10, 10, dw - 10, dh - 10], radius=14, fill=(8, 9, 12), outline=(35, 38, 46), width=2)

    # 2. Top Forehead
    # Wide speaker grill
    sp_w, sp_h = 120, 8
    draw.rounded_rectangle([dw // 2 - sp_w // 2, 30, dw // 2 + sp_w // 2, 30 + sp_h], radius=4, fill=(45, 48, 56), outline=(85, 90, 105), width=1)
    # Camera
    draw.ellipse([dw // 2 - 140, 28, dw // 2 - 124, 44], fill=(20, 25, 38), outline=(60, 68, 85), width=2)
    # Red LED
    draw.ellipse([dw // 2 + 126, 29, dw // 2 + 140, 43], fill=(220, 38, 38))
    # BlackBerry text
    f_logo = ImageFont.truetype(FONT_BOLD, 17)
    draw_centered_text(draw, (dw // 2, 60), "BlackBerry", f_logo, (215, 220, 230))

    # 3. Passport Screen
    sx = (dw - sw) // 2
    sy = 88
    mockup.paste(screen_img, (sx, sy))
    draw.rectangle([sx - 1, sy - 1, sx + sw, sy + sh], outline=(50, 55, 68), width=1)

    # 4. 3-Row Touch Keyboard
    ky = sy + sh + 18
    kh = 310
    row_h = kh // 3

    f_key = ImageFont.truetype(FONT_BOLD, 17)
    passport_rows = [
        ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
        ["A", "S", "D", "F", "G", "H", "J", "K", "L", "del"],
        ["Z", "X", "C", "V", "B", "N", "M", "↵"]
    ]

    for r_idx, r_keys in enumerate(passport_rows):
        ry = ky + r_idx * row_h
        # Stainless steel fret
        draw.line([(sx, ry), (sx + sw, ry)], fill=(185, 190, 202), width=3)

        kw = sw / float(len(r_keys))
        for k_idx, k_char in enumerate(r_keys):
            kx = sx + k_idx * kw
            draw.rounded_rectangle([kx + 3, ry + 6, kx + kw - 3, ry + row_h - 6], radius=4, fill=(24, 27, 34), outline=(42, 46, 56), width=1)
            draw_centered_text(draw, (kx + kw / 2, ry + row_h / 2), k_char, f_key, (230, 235, 245))

    draw.line([(sx, ky + kh), (sx + sw, ky + kh)], fill=(185, 190, 202), width=3)

    mockup.save(out_path, "PNG", quality=95)
    print(f"Saved Device Mockup: {out_path}")

# ==============================================================================
# SCREEN 9: ACTIVE FRAME PREVIEW (360 x 480)
# ==============================================================================
def render_active_frame_preview(out_path):
    w, h = 360, 480
    img = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # 1. Fullscreen Enlarged Book Cover
    cover_h = h - 48 # leave 48px for OS bottom bar
    # Draw an atmospheric stylized book cover
    draw.rectangle([0, 0, w, cover_h], fill=(18, 28, 24))
    # Simulated cover art / decorative pattern
    draw.ellipse([w // 2 - 90, cover_h // 2 - 130, w // 2 + 90, cover_h // 2 + 50], outline=(16, 185, 129), width=2)
    paste_icon(img, "assets/icons/ic_book.png", w // 2 - 40, cover_h // 2 - 80, (80, 80), tint_color=(16, 185, 129, 200))

    # Decorative cover text
    f_cov = ImageFont.truetype(FONT_BOLD, 22)
    draw_centered_text(draw, (w // 2, 70), "DANH GIA CỔ VẬT", f_cov, (243, 244, 246))

    # 2. Bottom Overlay Container (Dark semi-transparent #E6000000)
    ov_h = 86
    ov_y = cover_h - ov_h
    # Dark overlay box
    overlay_img = Image.new("RGBA", (w, ov_h), (0, 0, 0, 230))
    img.paste(overlay_img, (0, ov_y), overlay_img)

    # Book Title in Bottom Overlay
    f_title = ImageFont.truetype(FONT_BOLD, 17)
    draw.text((14, ov_y + 12), "Danh Gia Cổ Vật", font=f_title, fill=(255, 255, 255))
    draw.text((14, ov_y + 36), "#01 - Kỳ Án Đầu", font=f_title, fill=(255, 255, 255))

    # 3. Sleek Edge-to-Edge Progress Bar (5px height)
    pb_y = cover_h - 6
    pb_h = 6
    pct = 0.20 # 20%
    draw.rectangle([0, pb_y, w, pb_y + pb_h], fill=(34, 34, 34))
    draw.rectangle([0, pb_y, int(w * pct), pb_y + pb_h], fill=(16, 185, 129))

    # 4. OS Active Frame Bottom Bar (White card tab as seen in BB10)
    bar_y = cover_h
    draw.rectangle([0, bar_y, w, h], fill=(255, 255, 255))
    paste_icon(img, "assets/icons/ic_app_light.png", 14, bar_y + 12, (24, 24))
    f_bar = ImageFont.truetype(FONT_BOLD, 15)
    draw.text((46, bar_y + 14), "Libris", font=f_bar, fill=(30, 30, 30))

    # Close '✕' on right
    f_x = ImageFont.truetype(FONT_REG, 17)
    draw.text((w - 28, bar_y + 13), "✕", font=f_x, fill=(120, 120, 120))

    # Outer border
    draw.rectangle([0, 0, w - 1, h - 1], outline=(50, 50, 50), width=1)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# SCREEN 10: BOOKMARKS & NOTES PAGE (720x1280)
# ==============================================================================
def render_bookmarks_page(out_path):
    w, h = 720, 1280
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. BB10 Status Bar
    draw_bb10_status_bar(img, draw, w)

    # 2. Cascades TitleBar with Search Action
    draw_cascades_titlebar(img, draw, w, "Dấu Trang & Ghi Chú", y0=38, right_action={"icon": "assets/icons/ic_search.png"})

    # 3. Clean Single SegmentedControl (3 tabs: Tất cả / Dấu trang / Ghi chú)
    pad = 16
    sy = 108
    seg_w = w - pad * 2
    seg_h = 48
    draw.rounded_rectangle([pad, sy, pad + seg_w, sy + seg_h], radius=6, fill=(24, 24, 27), outline=(45, 45, 52), width=1)
    tab_w = seg_w // 3
    # Tab 1: Tất cả (Selected with Emerald indicator)
    draw.rounded_rectangle([pad + 2, sy + 2, pad + tab_w - 2, sy + seg_h - 2], radius=5, fill=(35, 40, 48))
    draw.line([(pad + 12, sy + seg_h - 4), (pad + tab_w - 12, sy + seg_h - 4)], fill=(16, 185, 129), width=3)
    f_tab = ImageFont.truetype(FONT_BOLD, 15)
    f_tab_un = ImageFont.truetype(FONT_REG, 15)
    draw_centered_text(draw, (pad + tab_w // 2, sy + seg_h // 2 - 2), "Tất cả", f_tab, (255, 255, 255))
    draw_centered_text(draw, (pad + tab_w + tab_w // 2, sy + seg_h // 2 - 2), "Dấu trang (0)", f_tab_un, (160, 160, 160))
    draw_centered_text(draw, (pad + tab_w * 2 + tab_w // 2, sy + seg_h // 2 - 2), "Ghi chú (0)", f_tab_un, (160, 160, 160))

    # 4. Compact Sub-header: Scope status & Emerald Switcher Pill
    sub_y = sy + seg_h + 12
    draw.text((pad + 4, sub_y + 8), "Phạm vi: Sách hiện tại", font=ImageFont.truetype(FONT_REG, 14), fill=(140, 140, 140))
    pill_w = 110
    pill_h = 36
    draw.rounded_rectangle([w - pad - pill_w, sub_y, w - pad, sub_y + pill_h], radius=6, fill=(24, 28, 36), outline=(42, 48, 60), width=1)
    draw_centered_text(draw, (w - pad - pill_w // 2, sub_y + pill_h // 2), "Tất cả sách", ImageFont.truetype(FONT_BOLD, 13), (16, 185, 129))

    # 5. Clean Empty State (Large centered bookmark icon, BigText title, centered subtitle)
    mid_y = 600
    ic_path = "assets/icons/ic_bookmarks.png"
    if os.path.exists(ic_path):
        orig_ic = Image.open(ic_path).convert("RGBA").resize((200, 200), Image.Resampling.LANCZOS)
        r, g, b, a = orig_ic.split()
        a = a.point(lambda p: int(p * 0.35))
        faded_ic = Image.merge("RGBA", (r, g, b, a))
        img.paste(faded_ic, (w // 2 - 100, mid_y - 140), faded_ic)

    draw_centered_text(draw, (w // 2, mid_y + 90), "Chưa có dấu trang hoặc ghi chú", ImageFont.truetype(FONT_BOLD, 22), (255, 255, 255))
    f_sub = ImageFont.truetype(FONT_REG, 15)
    draw_centered_text(draw, (w // 2, mid_y + 130), "Đánh dấu trang hoặc lưu trích dẫn khi đọc sách", f_sub, (150, 150, 150))
    draw_centered_text(draw, (w // 2, mid_y + 156), "để xem lại và tra cứu nhanh tại đây.", f_sub, (150, 150, 150))

    # 6. Cascades Action Bar
    actions = [
        {"title": "Tất cả sách", "icon": "assets/icons/ic_library.png"},
        {"title": "Đánh dấu", "icon": "assets/icons/ic_bookmark_add.png"},
        {"title": "Tủ sách", "icon": "assets/icons/ic_grid.png"}
    ]
    draw_cascades_action_bar(img, draw, w, h, actions)

    img.save(out_path, "PNG", quality=95)
    print(f"Saved: {out_path}")

# ==============================================================================
# MAIN ORCHESTRATION
# ==============================================================================
if __name__ == "__main__":
    out_dir = "/tmp"
    artifact_dir = "/home/Nhatu3rn/.gemini/antigravity/brain/7c15b93f-c67a-4662-b2cc-1663934be72a"

    # 1. Native BlackBerry 10 Cascades Screens (720 x 1280)
    previews = [
        ("preview_recent.png", render_recent_page),
        ("preview_reader.png", render_reader_page),
        ("preview_library_empty.png", render_library_empty_page),
        ("preview_library_populated.png", render_library_populated_page),
        ("preview_toc.png", render_toc_page),
        ("preview_settings.png", render_settings_page),
        ("preview_active_frame.png", render_active_frame_preview),
        ("preview_bookmarks.png", render_bookmarks_page)
    ]

    for fname, func in previews:
        tmp_path = os.path.join(out_dir, fname)
        func(tmp_path)
        art_path = os.path.join(artifact_dir, fname)
        shutil.copyfile(tmp_path, art_path)
        print(f"Copied to artifact: {art_path}")

    # 2. Square Screens (720x720) for Q10 / Classic / Passport
    sq_tmp = os.path.join(out_dir, "preview_square_library.png")
    render_square_library_page(sq_tmp)
    shutil.copyfile(sq_tmp, os.path.join(artifact_dir, "preview_square_library.png"))

    sq_reader_tmp = os.path.join(out_dir, "preview_square_reader.png")
    render_square_reader_page(sq_reader_tmp)
    shutil.copyfile(sq_reader_tmp, os.path.join(artifact_dir, "preview_square_reader.png"))

    # 3. Hardware Device Mockups
    classic_mockup_tmp = os.path.join(out_dir, "mockup_blackberry_classic.png")
    render_blackberry_classic_mockup(sq_tmp, classic_mockup_tmp)
    shutil.copyfile(classic_mockup_tmp, os.path.join(artifact_dir, "mockup_blackberry_classic.png"))

    passport_mockup_tmp = os.path.join(out_dir, "mockup_blackberry_passport.png")
    render_blackberry_passport_mockup(sq_reader_tmp, passport_mockup_tmp)
    shutil.copyfile(passport_mockup_tmp, os.path.join(artifact_dir, "mockup_blackberry_passport.png"))

    print("\nAll Authentic BlackBerry Previews and Hardware Mockups successfully rendered!")
