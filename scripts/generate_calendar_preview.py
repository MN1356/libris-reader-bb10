#!/usr/bin/env python3
"""
Generate high-fidelity preview of Libris Calendar & Reading Statistics UI.
"""
import math
import os
from PIL import Image, ImageDraw, ImageFont

FONT_BOLD = '/usr/share/fonts/open-sans/OpenSans-Bold.ttf'
FONT_REG = '/usr/share/fonts/open-sans/OpenSans-Regular.ttf'
FONT_SEMI = '/usr/share/fonts/open-sans/OpenSans-Semibold.ttf'

def get_text_size(draw, text, font):
    bbox = font.getbbox(text)
    return (bbox[2] - bbox[0], bbox[3] - bbox[1])

def draw_centered_text(draw, xy_center, text, font, fill):
    draw.text(xy_center, text, font=font, fill=fill, anchor='mm')
    bbox = font.getbbox(text)
    return (xy_center[0] - (bbox[2]-bbox[0])/2, xy_center[1] - (bbox[3]-bbox[1])/2, bbox[2]-bbox[0], bbox[3]-bbox[1])

def render_calendar_preview(out_path):
    w, h = 720, 1310
    img = Image.new('RGB', (w, h), (18, 18, 20)) # Dark AMOLED #121214
    draw = ImageDraw.Draw(img)

    f_title = ImageFont.truetype(FONT_BOLD, 22)
    f_header = ImageFont.truetype(FONT_BOLD, 18)
    f_sub = ImageFont.truetype(FONT_SEMI, 15)
    f_stat_val = ImageFont.truetype(FONT_BOLD, 22)
    f_stat_lbl = ImageFont.truetype(FONT_SEMI, 12)
    f_day_num = ImageFont.truetype(FONT_BOLD, 17)
    f_dur = ImageFont.truetype(FONT_BOLD, 15)
    f_card_title = ImageFont.truetype(FONT_BOLD, 16)
    f_card_sub = ImageFont.truetype(FONT_REG, 13)
    f_badge = ImageFont.truetype(FONT_BOLD, 11)

    pad = 16

    # 1. Sheet Title Bar (Native Cascades TitleBar style)
    draw.rectangle([0, 0, w, 60], fill=(24, 24, 27))
    draw.text((pad + 4, 18), 'Thống Kê Đọc Sách', font=f_title, fill=(255, 255, 255))
    draw.text((w - pad - 50, 20), 'Đóng', font=f_sub, fill=(16, 185, 129))
    draw.line([(0, 60), (w, 60)], fill=(39, 39, 42), width=1)

    # 2. View Mode Switcher: Theo Tháng | Theo Tuần (Clean, Symmetrical, No Broken Emojis)
    sw_y = 76
    sw_h = 42
    sw_w = w - pad * 2
    btn_gap = 10
    btn_w = (sw_w - btn_gap) // 2

    # Month button (Active, Emerald fill)
    m_btn_rect = [pad, sw_y, pad + btn_w, sw_y + sw_h]
    draw.rounded_rectangle(m_btn_rect, radius=6, fill=(16, 185, 129))
    draw_centered_text(draw, (pad + btn_w / 2, sw_y + sw_h / 2), 'Theo Tháng', f_sub, (255, 255, 255))

    # Week button (Inactive, Subtle dark border)
    w_btn_rect = [pad + btn_w + btn_gap, sw_y, pad + btn_w * 2 + btn_gap, sw_y + sw_h]
    draw.rounded_rectangle(w_btn_rect, radius=6, fill=(24, 24, 27), outline=(63, 63, 70), width=1)
    draw_centered_text(draw, (pad + btn_w + btn_gap + btn_w / 2, sw_y + sw_h / 2), 'Theo Tuần', f_sub, (161, 161, 170))

    # 3. Month Navigator Bar (100% Symmetrical: 92px Left [ Hôm nay ] | Center Title | 92px Right Stepper [ ‹ ] [ › ])
    nav_y = sw_y + sw_h + 14
    nav_h = 50
    draw.rounded_rectangle([pad, nav_y, w - pad, nav_y + nav_h], radius=8, fill=(24, 24, 27), outline=(39, 39, 42), width=1)
    
    # 3A. Today Button on Left (Width: 92px, Height: 34px, strictly centered at nav_y + nav_h/2)
    t_w, t_h = 92, 34
    t_x1 = pad + 10
    t_y1 = nav_y + (nav_h - t_h) / 2
    t_x2 = t_x1 + t_w
    t_y2 = t_y1 + t_h
    draw.rounded_rectangle([t_x1, t_y1, t_x2, t_y2], radius=6, fill=(35, 35, 40), outline=(55, 55, 62), width=1)
    draw.text(((t_x1 + t_x2) / 2, (t_y1 + t_y2) / 2), 'Hôm nay', font=ImageFont.truetype(FONT_SEMI, 13), fill=(255, 255, 255), anchor='mm')
    
    # 3B. Month Title (Dead Center of Screen: x = w / 2 = 360)
    draw.text((w / 2, nav_y + nav_h / 2), 'Tháng 11, 2026', font=f_header, fill=(255, 255, 255), anchor='mm')
    
    # 3C. Navigation Stepper [ ‹ ] [ › ] on Right (Total Width: 42 + 8 + 42 = 92px, matching Left!)
    s_w, s_h = 42, 34
    s_gap = 8
    s_r = w - pad - 10
    # Next [ › ]
    nx1 = s_r - s_w
    draw.rounded_rectangle([nx1, nav_y + (nav_h - s_h) / 2, s_r, nav_y + (nav_h + s_h) / 2], radius=6, fill=(35, 35, 40), outline=(55, 55, 62), width=1)
    draw.text((nx1 + s_w / 2, nav_y + nav_h / 2), '›', font=ImageFont.truetype(FONT_BOLD, 17), fill=(16, 185, 129), anchor='mm')
    # Prev [ ‹ ]
    px1 = nx1 - s_gap - s_w
    draw.rounded_rectangle([px1, nav_y + (nav_h - s_h) / 2, px1 + s_w, nav_y + (nav_h + s_h) / 2], radius=6, fill=(35, 35, 40), outline=(55, 55, 62), width=1)
    draw.text((px1 + s_w / 2, nav_y + nav_h / 2), '‹', font=ImageFont.truetype(FONT_BOLD, 17), fill=(16, 185, 129), anchor='mm')

    # 4. 2x2 Summary Metric Cards (Harmonious layout)
    m_y = nav_y + nav_h + 12
    card_h = 68
    for r in range(2):
        for c in range(2):
            cx1 = pad + c * (btn_w + btn_gap)
            cy1 = m_y + r * (card_h + 8)
            draw.rounded_rectangle([cx1, cy1, cx1 + btn_w, cy1 + card_h], radius=6, fill=(24, 24, 27), outline=(39, 39, 42), width=1)
            if r == 0 and c == 0:
                draw.text((cx1 + 12, cy1 + 8), 'TỔNG THỜI GIAN THÁNG', font=f_stat_lbl, fill=(16, 185, 129))
                draw.text((cx1 + 12, cy1 + 28), '18h 45m', font=f_stat_val, fill=(255, 255, 255))
            elif r == 0 and c == 1:
                draw.text((cx1 + 12, cy1 + 8), 'SỐ NGÀY ĐỌC', font=f_stat_lbl, fill=(161, 161, 170))
                draw.text((cx1 + 12, cy1 + 28), '12 / 30 (40%)', font=f_stat_val, fill=(255, 255, 255))
            elif r == 1 and c == 0:
                draw.text((cx1 + 12, cy1 + 8), 'TRUNG BÌNH / NGÀY', font=f_stat_lbl, fill=(161, 161, 170))
                draw.text((cx1 + 12, cy1 + 28), '1h 33m', font=f_stat_val, fill=(56, 189, 248))
            elif r == 1 and c == 1:
                draw.text((cx1 + 12, cy1 + 8), 'CHUỖI DÀI NHẤT', font=f_stat_lbl, fill=(161, 161, 170))
                draw.text((cx1 + 12, cy1 + 28), '5 ngày liên tục', font=f_stat_val, fill=(245, 158, 11))

    # 5. Calendar 7-column Grid Card
    cal_y = m_y + (card_h + 8) * 2 + 6
    cal_w = w - pad * 2
    cal_h = 662
    draw.rounded_rectangle([pad, cal_y, w - pad, cal_y + cal_h], radius=8, fill=(24, 24, 27), outline=(39, 39, 42), width=1)

    # Weekday Header
    weekdays = ['T2', 'T3', 'T4', 'T5', 'T6', 'T7', 'CN']
    col_w = (cal_w - 14) // 7
    hdr_y = cal_y + 10
    for i, day_name in enumerate(weekdays):
        col_center_x = pad + 7 + i * col_w + col_w / 2
        color = (251, 113, 133) if i == 6 else (161, 161, 170)
        draw_centered_text(draw, (col_center_x, hdr_y + 8), day_name, f_sub, color)

    # Divider below weekday header
    draw.line([(pad + 6, hdr_y + 24), (w - pad - 6, hdr_y + 24)], fill=(39, 39, 42), width=1)

    # Grid Cells (6 rows x 7 cols)
    grid_y = hdr_y + 30
    cell_h = 100
    selected_day = 14
    today_day = 14
    reading_days = {
        2: '35m', 3: '1h 10m', 5: '45m', 6: '2h', 7: '1h 15m',
        9: '50m', 10: '1h 40m', 11: '25m', 12: '1h 05m', 13: '40m', 14: '1h 20m'
    }

    start_offset = 6 # Nov 1, 2026 starts on Sunday (col 6)
    day_num = 1
    for r in range(6):
        for c in range(7):
            idx = r * 7 + c
            cx1 = pad + 7 + c * col_w
            cy1 = grid_y + r * cell_h
            cx2 = cx1 + col_w - 2
            cy2 = cy1 + cell_h - 2
            cell_center_x = (cx1 + cx2) / 2

            if idx < start_offset:
                # Prev month
                p_day = 26 + idx
                draw.rectangle([cx1, cy1, cx2, cy2], fill=(18, 18, 20))
                draw_centered_text(draw, (cell_center_x, cy1 + 14), str(p_day), f_day_num, (63, 63, 70))
            elif day_num <= 30:
                cur_d = day_num
                has_read = cur_d in reading_days
                is_sel = (cur_d == selected_day)
                is_td = (cur_d == today_day)

                # Cell background
                if is_sel:
                    bg = (6, 78, 59) # Deep emerald selected
                    border = (16, 185, 129)
                elif has_read:
                    bg = (19, 42, 33) # Harmonious emerald heat tint
                    border = (39, 39, 42)
                else:
                    bg = (20, 20, 23)
                    border = (39, 39, 42)

                draw.rounded_rectangle([cx1, cy1, cx2, cy2], radius=4, fill=bg, outline=border, width=1)

                # Day number - STRICTLY CENTERED HORIZONTALLY
                num_color = (255, 255, 255) if (has_read or is_sel) else (212, 212, 216)
                if is_td:
                    # Today indicator badge (perfectly centered circle)
                    badge_size = 26
                    draw.ellipse([cell_center_x - badge_size/2, cy1 + 4, cell_center_x + badge_size/2, cy1 + 4 + badge_size], fill=(16, 185, 129))
                    draw_centered_text(draw, (cell_center_x, cy1 + 4 + badge_size/2), str(cur_d), f_day_num, (255, 255, 255))
                else:
                    draw_centered_text(draw, (cell_center_x, cy1 + 14), str(cur_d), f_day_num, num_color)

                # Duration pill - STRICTLY CENTERED HORIZONTALLY AT BOTTOM (Large, Crisp Font)
                if has_read:
                    dur_str = reading_days[cur_d]
                    tbw, tbh = get_text_size(draw, dur_str, f_dur)
                    pill_w = max(tbw + 18, 50)
                    pill_h = 23
                    pill_x1 = cell_center_x - pill_w / 2
                    pill_x2 = cell_center_x + pill_w / 2
                    pill_y1 = cy2 - pill_h - 4
                    pill_y2 = cy2 - 4
                    pill_bg = (16, 185, 129) if is_sel else (6, 95, 70)
                    draw.rounded_rectangle([pill_x1, pill_y1, pill_x2, pill_y2], radius=5, fill=pill_bg)
                    draw.text((cell_center_x, (pill_y1 + pill_y2) / 2), dur_str, font=f_dur, fill=(255, 255, 255), anchor='mm')

                day_num += 1
            else:
                # Next month
                n_day = day_num - 30
                draw.rectangle([cx1, cy1, cx2, cy2], fill=(18, 18, 20))
                draw_centered_text(draw, (cell_center_x, cy1 + 14), str(n_day), f_day_num, (63, 63, 70))
                day_num += 1

    # 6. Selected Day Details Card (Refined per User Feedback)
    # Feedback:
    # 1) Title: "Ngày 14/11/2026"
    # 2) Book cover & title/author/progress stretch smoothly to the right
    # 3) Reading duration ("Đọc: 1h 20m") placed level with author name
    # 4) Page counter ("Trang 120 / 350")
    # 5) Book format ("EPUB") placed on the right side
    det_y = cal_y + cal_h + 14
    det_h = 176
    draw.rounded_rectangle([pad, det_y, w - pad, det_y + det_h], radius=8, fill=(24, 24, 27), outline=(39, 39, 42), width=1)

    # Top header of card: "Ngày 14/11/2026"
    draw.text((pad + 14, det_y + 12), 'Ngày 14/11/2026', font=f_header, fill=(255, 255, 255))
    draw.text((pad + 14, det_y + 36), 'Tổng thời gian đọc: 1h 20m', font=f_stat_lbl, fill=(16, 185, 129))

    # Book slot card (streches smoothly across)
    bk_y = det_y + 60
    bk_h = 96
    card_right = w - pad - 12
    draw.rounded_rectangle([pad + 12, bk_y, card_right, bk_y + bk_h], radius=6, fill=(18, 18, 20), outline=(45, 45, 48), width=1)

    # Book Cover (Left: 50x74)
    cov_x = pad + 20
    cov_y = bk_y + 11
    cov_w = 50
    cov_h = 74
    draw.rounded_rectangle([cov_x, cov_y, cov_x + cov_w, cov_y + cov_h], radius=4, fill=(6, 78, 59), outline=(16, 185, 129), width=1)
    # Book spine graphic & cover lines
    draw.rectangle([cov_x + 6, cov_y + 4, cov_x + 9, cov_y + cov_h - 4], fill=(16, 185, 129))
    draw.line([(cov_x + 14, cov_y + 18), (cov_x + cov_w - 8, cov_y + 18)], fill=(255, 255, 255), width=2)
    draw.line([(cov_x + 14, cov_y + 25), (cov_x + cov_w - 12, cov_y + 25)], fill=(255, 255, 255), width=1)
    draw.line([(cov_x + 14, cov_y + 32), (cov_x + cov_w - 16, cov_y + 32)], fill=(255, 255, 255), width=1)
    draw.text((cov_x + 14, cov_y + cov_h - 18), 'EPUB', font=ImageFont.truetype(FONT_BOLD, 8), fill=(16, 185, 129))

    # Details column (streches across to card_right - 80)
    col_x = cov_x + cov_w + 14
    content_right = card_right - 82

    # Row 1: Book Title (left) & Format badge (right)
    # Format badge on far right of title row
    fmt_text = 'EPUB'
    fbw, fbh = get_text_size(draw, fmt_text, f_badge)
    fmt_x2 = content_right
    fmt_x1 = fmt_x2 - fbw - 14
    fmt_y1 = bk_y + 12
    fmt_y2 = fmt_y1 + 18
    draw.rounded_rectangle([fmt_x1, fmt_y1, fmt_x2, fmt_y2], radius=4, fill=(16, 185, 129))
    draw_centered_text(draw, ((fmt_x1 + fmt_x2) / 2, (fmt_y1 + fmt_y2) / 2), fmt_text, f_badge, (255, 255, 255))

    # Book Title (truncated before format badge)
    draw.text((col_x, bk_y + 12), 'Nhà Giả Kim (The Alchemist)', font=f_card_title, fill=(255, 255, 255))

    # Row 2: Author (left) & Reading Duration (right, exactly level with author!)
    row2_y = bk_y + 36
    draw.text((col_x, row2_y), 'Paulo Coelho', font=f_card_sub, fill=(161, 161, 170))
    # Duration on right level with author
    dur_label = 'Đọc: 1h 20m'
    dbw, _ = get_text_size(draw, dur_label, f_dur)
    draw.text((content_right - dbw, row2_y + 1), dur_label, font=f_dur, fill=(16, 185, 129))

    # Row 3: Progress Bar & Page Counter
    row3_y = bk_y + 64
    bar_w = content_right - col_x - 140
    bar_h = 6
    # Progress track background
    draw.rounded_rectangle([col_x, row3_y + 5, col_x + bar_w, row3_y + 5 + bar_h], radius=3, fill=(39, 39, 42))
    # Progress track fill (70%)
    fill_w = int(bar_w * 0.70)
    draw.rounded_rectangle([col_x, row3_y + 5, col_x + fill_w, row3_y + 5 + bar_h], radius=3, fill=(16, 185, 129))

    # Page Counter & Percentage
    draw.text((col_x + bar_w + 10, row3_y + 1), 'Trang 120 / 350 • 70%', font=f_dur, fill=(161, 161, 170))

    # Action Button "Đọc" on right edge
    btn_x1 = card_right - 68
    btn_y1 = bk_y + 26
    btn_x2 = card_right - 10
    btn_y2 = bk_y + 70
    draw.rounded_rectangle([btn_x1, btn_y1, btn_x2, btn_y2], radius=6, fill=(16, 185, 129))
    draw_centered_text(draw, ((btn_x1 + btn_x2) / 2, (btn_y1 + btn_y2) / 2), 'Đọc', f_card_title, (255, 255, 255))

    img.save(out_path)
    print(f'Rendered preview saved to: {out_path}')

if __name__ == '__main__':
    render_calendar_preview('/home/Nhatu3rn/.gemini/antigravity/brain/b8fb5a67-1d2b-45f5-8576-7f6ac818944a/calendar_preview.png')
