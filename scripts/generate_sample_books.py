#!/usr/bin/env python3
"""Generate lightweight sample books for every supported Libris format."""
import base64
import ctypes
import io
import os
import struct
import zipfile
from PIL import Image, ImageDraw

OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "samples")


def make_cover_image(title_line1, title_line2, subtitle, accent=(16, 185, 129), bg=(18, 26, 38)):
    img = Image.new("RGB", (360, 480), bg)
    d = ImageDraw.Draw(img)
    d.rectangle([14, 14, 346, 466], outline=accent, width=3)
    d.rectangle([28, 36, 332, 165], fill=accent)
    d.text((44, 68), "LIBRIS", fill=(255, 255, 255))
    d.text((44, 102), title_line1, fill=(240, 255, 250))
    d.text((44, 126), title_line2, fill=(230, 245, 240))
    d.rectangle([40, 210, 320, 214], fill=accent)
    d.text((40, 236), subtitle, fill=(220, 230, 240))
    d.text((40, 268), "BlackBerry 10 E-Book Reader", fill=(160, 178, 196))
    d.rectangle([40, 340, 320, 420], outline=(80, 100, 120), width=1)
    d.text((56, 364), "Sample Edition - Build 200+", fill=(180, 200, 215))
    d.text((56, 386), "Lightweight Test Book", fill=accent)
    return img


def make_cover_jpeg(title_line1, title_line2, subtitle, accent=(16, 185, 129), bg=(18, 26, 38)):
    img = make_cover_image(title_line1, title_line2, subtitle, accent=accent, bg=bg)
    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=82)
    return buf.getvalue()


def make_figure_png(label, bg=(245, 248, 250), accent=(14, 165, 233)):
    img = Image.new("RGB", (320, 160), bg)
    d = ImageDraw.Draw(img)
    d.rectangle([8, 8, 312, 152], outline=accent, width=3)
    d.rectangle([24, 24, 96, 136], fill=accent)
    d.ellipse([36, 50, 84, 98], fill=(255, 255, 255))
    d.text((114, 48), label, fill=(20, 30, 45))
    d.text((114, 76), "Embedded Image Rendering OK", fill=(60, 80, 100))
    d.text((114, 104), "Libris WebView / Data URI", fill=accent)
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def build_epub(out_path):
    cover_jpg = make_cover_jpeg("EPUB 3 SAMPLE", "COVER & NOTES", "Format: EPUB (.epub)")
    fig_png = make_figure_png("EPUB Illustration")

    container_xml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">\n'
        '  <rootfiles>\n'
        '    <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>\n'
        '  </rootfiles>\n'
        '</container>\n'
    )
    content_opf = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<package xmlns="http://www.idpf.org/2007/opf" unique-identifier="bookid" version="2.0">\n'
        '  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:opf="http://www.idpf.org/2007/opf">\n'
        '    <dc:title>Libris Sample (EPUB)</dc:title>\n'
        '    <dc:creator opf:role="aut">Libris Team</dc:creator>\n'
        '    <dc:language>vi</dc:language>\n'
        '    <dc:subject>Demo</dc:subject>\n'
        '    <dc:description>Sách mẫu định dạng EPUB kèm ảnh bìa, ảnh minh họa và chú thích dấu *.</dc:description>\n'
        '    <meta name="cover" content="cover-image"/>\n'
        '  </metadata>\n'
        '  <manifest>\n'
        '    <item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>\n'
        '    <item id="cover-image" href="Images/cover.jpg" media-type="image/jpeg"/>\n'
        '    <item id="fig-1" href="Images/fig1.png" media-type="image/png"/>\n'
        '    <item id="cover-page" href="Text/cover.xhtml" media-type="application/xhtml+xml"/>\n'
        '    <item id="chap1" href="Text/chap01.xhtml" media-type="application/xhtml+xml"/>\n'
        '    <item id="footnotes" href="Text/footnotes.xhtml" media-type="application/xhtml+xml"/>\n'
        '  </manifest>\n'
        '  <spine toc="ncx">\n'
        '    <itemref idref="cover-page"/>\n'
        '    <itemref idref="chap1"/>\n'
        '    <itemref idref="footnotes"/>\n'
        '  </spine>\n'
        '</package>\n'
    )
    toc_ncx = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/" version="2005-1">\n'
        '  <head><meta name="dtb:uid" content="libris-sample-epub"/></head>\n'
        '  <docTitle><text>Libris Sample (EPUB)</text></docTitle>\n'
        '  <navMap>\n'
        '    <navPoint id="nav0" playOrder="1"><navLabel><text>Bìa sách</text></navLabel><content src="Text/cover.xhtml"/></navPoint>\n'
        '    <navPoint id="nav1" playOrder="2"><navLabel><text>Chương 1: Kiểm tra Ảnh &amp; Chú thích</text></navLabel><content src="Text/chap01.xhtml"/></navPoint>\n'
        '    <navPoint id="nav2" playOrder="3"><navLabel><text>Phụ lục Chú thích</text></navLabel><content src="Text/footnotes.xhtml"/></navPoint>\n'
        '  </navMap>\n'
        '</ncx>\n'
    )
    cover_xhtml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">\n'
        '<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Bìa sách</title></head>\n'
        '<body style="text-align:center;">\n'
        '  <div><img src="../Images/cover.jpg" alt="Book Cover"/></div>\n'
        '</body></html>\n'
    )
    chap1_xhtml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">\n'
        '<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops">\n'
        '<head><title>Chương 1: Kiểm tra Ảnh &amp; Chú thích</title></head>\n'
        '<body>\n'
        '  <h1>Chương 1: Kiểm tra Ảnh &amp; Chú thích</h1>\n'
        '  <p>Đây là đoạn văn mẫu tiếng Việt dùng để kiểm tra khả năng hiển thị ảnh nhúng và dấu chú thích<a id="fnref_cross" href="footnotes.xhtml#fn_cross"><sup>*</sup></a> nằm ở tệp riêng.</p>\n'
        '  <p>Bạn cũng có thể chạm vào chú thích cùng trang<a id="fnref_local" class="noteref" href="#fn_local"><sup>[1]</sup></a> để xem popup bật lên ngay trên trang đọc mà không gặp lỗi 404.</p>\n'
        '  <p><img src="../Images/fig1.png" alt="Minh họa EPUB"/></p>\n'
        '  <aside id="fn_local" class="footnote"><p>[1] Chú thích cùng chương: hiển thị ngay dưới dạng thẻ nổi trong trình đọc.</p></aside>\n'
        '</body></html>\n'
    )
    footnotes_xhtml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">\n'
        '<html xmlns="http://www.w3.org/1999/xhtml">\n'
        '<head><title>Phụ lục Chú thích</title></head>\n'
        '<body>\n'
        '  <h2>Phụ lục Chú thích</h2>\n'
        '  <div id="fn_cross" class="footnote"><p>* Đây là nội dung chú thích nằm ở tệp footnotes.xhtml riêng biệt. Nhấn vào dấu * trong Chương 1 sẽ đọc được trực tiếp mà không báo lỗi 404.</p></div>\n'
        '</body></html>\n'
    )

    with zipfile.ZipFile(out_path, "w") as zf:
        zf.writestr("mimetype", b"application/epub+zip", compress_type=zipfile.ZIP_STORED)
        zf.writestr("META-INF/container.xml", container_xml.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("OEBPS/content.opf", content_opf.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("OEBPS/toc.ncx", toc_ncx.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("OEBPS/Images/cover.jpg", cover_jpg, compress_type=zipfile.ZIP_STORED)
        zf.writestr("OEBPS/Images/fig1.png", fig_png, compress_type=zipfile.ZIP_STORED)
        zf.writestr("OEBPS/Text/cover.xhtml", cover_xhtml.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("OEBPS/Text/chap01.xhtml", chap1_xhtml.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("OEBPS/Text/footnotes.xhtml", footnotes_xhtml.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)


def build_fb2(out_path):
    cover_jpg = make_cover_jpeg("FB2 SAMPLE", "FICTIONBOOK 2.0", "Format: FB2 (.fb2)", accent=(245, 158, 11))
    fig_png = make_figure_png("FB2 Illustration", accent=(245, 158, 11))
    b64_cov = base64.b64encode(cover_jpg).decode("ascii")
    b64_fig = base64.b64encode(fig_png).decode("ascii")

    xml = (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink">\n'
        '  <description>\n'
        '    <title-info>\n'
        '      <genre>sf</genre>\n'
        '      <author><first-name>Libris</first-name><last-name>Team</last-name></author>\n'
        '      <book-title>Libris Sample (FB2)</book-title>\n'
        '      <annotation><p>Sách mẫu định dạng FictionBook 2.0 kèm ảnh bìa, ảnh minh họa và chú thích.</p></annotation>\n'
        '      <coverpage><image l:href="#cover.jpg"/></coverpage>\n'
        '      <lang>vi</lang>\n'
        '      <sequence name="Libris Demo" number="1"/>\n'
        '    </title-info>\n'
        '  </description>\n'
        '  <body>\n'
        '    <section>\n'
        '      <title><p>Chương 1: Giới thiệu FB2</p></title>\n'
        '      <p>Đây là nội dung sách mẫu FictionBook 2.0. Nhấp vào ký hiệu chú thích <a l:href="#n_1" type="note">*</a> để mở nội dung từ phần notes.</p>\n'
        '      <image l:href="#fig1.png"/>\n'
        '    </section>\n'
        '    <section>\n'
        '      <title><p>Chương 2: Hiển thị tiếng Việt</p></title>\n'
        '      <p>Libris hỗ trợ đầy đủ tiếng Việt Unicode, EPUB, FB2, MOBI, CBZ, PDF, DjVu và TXT trên BlackBerry 10.</p>\n'
        '    </section>\n'
        '  </body>\n'
        '  <body name="notes">\n'
        '    <section id="n_1">\n'
        '      <title><p>1</p></title>\n'
        '      <p>* Chú thích mẫu FB2: được phân giải trực tiếp từ thẻ body name="notes".</p>\n'
        '    </section>\n'
        '  </body>\n'
        f'  <binary id="cover.jpg" content-type="image/jpeg">{b64_cov}</binary>\n'
        f'  <binary id="fig1.png" content-type="image/png">{b64_fig}</binary>\n'
        '</FictionBook>\n'
    )
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(xml)


def build_cbz(out_path):
    p1 = make_cover_jpeg("CBZ COMIC", "SAMPLE VOLUME 1", "Format: CBZ (.cbz)", accent=(6, 182, 212))
    p2 = make_figure_png("CBZ Page 2 - Comic Panel", accent=(6, 182, 212))
    p3 = make_figure_png("CBZ Page 3 - Manga RTL Test", accent=(139, 92, 246))
    comic_info = (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<ComicInfo>\n'
        '  <Title>Libris Sample (CBZ)</Title>\n'
        '  <Series>Libris Demo</Series>\n'
        '  <Number>1</Number>\n'
        '  <Writer>Libris Team</Writer>\n'
        '  <Genre>Comic / Manga</Genre>\n'
        '  <Summary>Truyện tranh mẫu CBZ 3 trang kèm ComicInfo.xml.</Summary>\n'
        '</ComicInfo>\n'
    )
    with zipfile.ZipFile(out_path, "w") as zf:
        zf.writestr("ComicInfo.xml", comic_info.encode("utf-8"), compress_type=zipfile.ZIP_DEFLATED)
        zf.writestr("01_cover.jpg", p1, compress_type=zipfile.ZIP_STORED)
        zf.writestr("02_page.png", p2, compress_type=zipfile.ZIP_STORED)
        zf.writestr("03_page.png", p3, compress_type=zipfile.ZIP_STORED)


def build_txt(out_path):
    content = (
        "Libris Sample (TXT)\n"
        "Tác giả: Libris Team\n\n"
        "Chương 1: Khởi đầu với văn bản thuần TXT\n\n"
        "Đây là tệp văn bản thuần UTF-8 mẫu dùng để kiểm tra bộ đọc TXT của Libris trên BlackBerry 10.\n"
        "Trình đọc tự động nhận diện mã hóa tiếng Việt (UTF-8, UTF-16, TCVN3, VNI, Windows-1258) và tự động tách mục lục theo tên chương.\n\n"
        "Chương 2: Tính năng đọc sách và từ điển\n\n"
        "Bạn có thể bôi đen từ vựng bất kỳ để tra từ điển ngoại tuyến, dịch câu, tô sáng (highlight) hoặc nghe đọc bằng giọng nói TTS.\n\n"
        "Chương 3: Kết luận\n\n"
        "Chúc bạn có trải nghiệm đọc sách mượt mà trên BlackBerry 10!\n"
    )
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(content)


def build_mobi(out_path):
    cover_jpg = make_cover_jpeg("MOBI SAMPLE", "KINDLE FORMAT", "Format: MOBI (.mobi)", accent=(139, 92, 246))
    fig_jpg_img = Image.open(io.BytesIO(make_figure_png("MOBI Embedded Image", accent=(139, 92, 246)))).convert("RGB")
    fig_buf = io.BytesIO()
    fig_jpg_img.save(fig_buf, format="JPEG", quality=82)
    fig_jpg = fig_buf.getvalue()

    html_str = (
        "<html><head><title>Libris Sample (MOBI)</title></head><body>"
        "<h1>Chương 1: Sách mẫu MOBI</h1>"
        "<p>Đây là nội dung sách mẫu MOBI UTF-8 có ảnh bìa EXTH, ảnh minh họa nhúng và chú thích<a href=\"#fn_mobi\">*</a>.</p>"
        "<p><img recindex=\"00002\"/></p>"
        "<mbp:pagebreak/>"
        "<h1>Chương 2: Ghi chú MOBI</h1>"
        "<aside id=\"fn_mobi\"><p>* Nội dung chú thích MOBI nằm ở chương 2 và được hiển thị ngay bằng popup.</p></aside>"
        "</body></html>"
    )
    raw_text = html_str.encode("utf-8")

    # Split uncompressed PalmDOC text into 4096-byte records (compression = 1: no compression)
    text_records = [raw_text[i:i + 4096] for i in range(0, len(raw_text), 4096)]
    if not text_records:
        text_records = [b"<html><body></body></html>"]

    def make_exth_record(rec_type, data_bytes):
        rec_len = 8 + len(data_bytes)
        return struct.pack(">II", rec_type, rec_len) + data_bytes

    title_bytes = "Libris Sample (MOBI)".encode("utf-8")
    exth_entries = b"".join([
        make_exth_record(100, "Libris Team".encode("utf-8")),
        make_exth_record(103, "Sách mẫu định dạng MOBI kèm ảnh bìa, ảnh minh họa và chú thích.".encode("utf-8")),
        make_exth_record(105, "Demo".encode("utf-8")),
        make_exth_record(201, struct.pack(">I", 0)),  # cover offset relative to firstImageIndex
        make_exth_record(503, title_bytes),
    ])
    exth_total_len = 12 + len(exth_entries)
    pad_len = (4 - (exth_total_len % 4)) % 4
    exth_block = b"EXTH" + struct.pack(">II", exth_total_len + pad_len, 5) + exth_entries + (b"\x00" * pad_len)

    first_image_index = 1 + len(text_records)
    mobi_header_len = 232
    full_name_offset = 16 + mobi_header_len + len(exth_block)
    full_name_length = len(title_bytes)

    # Record 0: 16-byte PalmDOC header + 232-byte MOBI header + EXTH + fullName
    palmdoc_hdr = struct.pack(">HHIHHHH", 1, 0, len(raw_text), len(text_records), 4096, 0, 0)
    mobi_hdr = bytearray(mobi_header_len)
    mobi_hdr[0:4] = b"MOBI"
    struct.pack_into(">I", mobi_hdr, 4, mobi_header_len)   # mobiHeaderLen (at rec0 offset 20)
    struct.pack_into(">I", mobi_hdr, 8, 2)                 # mobi type
    struct.pack_into(">I", mobi_hdr, 12, 65001)            # UTF-8
    struct.pack_into(">I", mobi_hdr, 16, 1)                # uid
    struct.pack_into(">I", mobi_hdr, 20, 6)                # file version 6
    for off in range(24, 64, 4):
        struct.pack_into(">I", mobi_hdr, off, 0xFFFFFFFF)
    struct.pack_into(">I", mobi_hdr, 68, full_name_offset) # rec0 offset 84
    struct.pack_into(">I", mobi_hdr, 72, full_name_length) # rec0 offset 88
    struct.pack_into(">I", mobi_hdr, 76, 1066)             # locale vi
    struct.pack_into(">I", mobi_hdr, 92, first_image_index) # rec0 offset 108
    struct.pack_into(">I", mobi_hdr, 96, 0xFFFFFFFF)       # huff rec idx (offset 112)
    struct.pack_into(">I", mobi_hdr, 100, 0)               # huff count
    struct.pack_into(">I", mobi_hdr, 112, 0x40)            # exth flags (rec0 offset 128)
    struct.pack_into(">H", mobi_hdr, 226, 0)               # extraFlags (rec0 offset 242)

    rec0 = palmdoc_hdr + bytes(mobi_hdr) + exth_block + title_bytes + b"\x00\x00"
    all_records = [rec0] + text_records + [cover_jpg, fig_jpg]

    num_records = len(all_records)
    pdb_name = b"Libris_Sample_MOBI".ljust(32, b"\x00")[:32]
    # 78-byte PalmDB header + 8 bytes per record entry + 2 gap bytes
    data_start = 78 + num_records * 8 + 2
    pdb_hdr = (
        pdb_name
        + struct.pack(">HHIIIIII", 0, 0, 0, 0, 0, 0, 0, 0)
        + b"BOOKMOBI"
        + struct.pack(">IIH", 0, 0, num_records)
    )
    assert len(pdb_hdr) == 78

    offsets_table = bytearray()
    cur_off = data_start
    for idx, rdata in enumerate(all_records):
        offsets_table += struct.pack(">IB3s", cur_off, 0, struct.pack(">I", idx)[1:])
        cur_off += len(rdata)

    with open(out_path, "wb") as f:
        f.write(pdb_hdr)
        f.write(offsets_table)
        f.write(b"\x00\x00")
        for rdata in all_records:
            f.write(rdata)


def build_pdf(out_path):
    cover_jpg = make_cover_jpeg("PDF SAMPLE", "VECTOR & TEXT", "Format: PDF (.pdf)", accent=(239, 68, 68))
    cov_w, cov_h = 360, 480

    page1_stream = (
        "q 595 0 0 842 0 0 cm 0.08 0.11 0.16 rg 0 0 1 1 re f Q\n"
        "q 420 0 0 560 87 180 cm /Im1 Do Q\n"
        "BT /F1 18 Tf 0.95 0.96 0.98 rg 87 130 Td (Libris Sample PDF - BlackBerry 10) Tj ET\n"
        "BT /F1 12 Tf 0.70 0.78 0.85 rg 87 104 Td (Page 1: Cover Visual + Reflow Text Layer Ready) Tj ET\n"
    ).encode("latin1")

    page2_stream = (
        "BT\n"
        "/F1 20 Tf 72 760 Td (Chapter 1: Libris PDF Reflow & Vector Test) Tj\n"
        "/F1 12 Tf 0 -36 Td (This lightweight 2-page PDF validates native vector rendering,) Tj\n"
        "0 -20 Td (page 1 cover thumbnail extraction, and Text Reflow mode in Libris.) Tj\n"
        "0 -28 Td (Try tapping Text Reflow or Search inside Reader menu to inspect text.) Tj\n"
        "ET\n"
    ).encode("latin1")

    objs = []
    objs.append(b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n")
    objs.append(b"2 0 obj\n<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>\nendobj\n")
    objs.append(
        b"3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] "
        b"/Resources << /Font << /F1 5 0 R >> /XObject << /Im1 6 0 R >> >> "
        b"/Contents 7 0 R >>\nendobj\n"
    )
    objs.append(
        b"4 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] "
        b"/Resources << /Font << /F1 5 0 R >> >> "
        b"/Contents 8 0 R >>\nendobj\n"
    )
    objs.append(b"5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n")
    objs.append(
        f"6 0 obj\n<< /Type /XObject /Subtype /Image /Width {cov_w} /Height {cov_h} "
        f"/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length {len(cover_jpg)} >>\nstream\n".encode("latin1")
        + cover_jpg + b"\nendstream\nendobj\n"
    )
    objs.append(
        f"7 0 obj\n<< /Length {len(page1_stream)} >>\nstream\n".encode("latin1")
        + page1_stream + b"\nendstream\nendobj\n"
    )
    objs.append(
        f"8 0 obj\n<< /Length {len(page2_stream)} >>\nstream\n".encode("latin1")
        + page2_stream + b"\nendstream\nendobj\n"
    )

    pdf = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
    offsets = [0]
    for obj in objs:
        offsets.append(len(pdf))
        pdf.extend(obj)
    xref_pos = len(pdf)
    pdf.extend(f"xref\n0 {len(objs) + 1}\n0000000000 65535 f \n".encode("latin1"))
    for off in offsets[1:]:
        pdf.extend(f"{off:010d} 00000 n \n".encode("latin1"))
    pdf.extend(
        f"trailer\n<< /Size {len(objs) + 1} /Root 1 0 R "
        f"/Info << /Title (Libris Sample PDF) /Author (Libris Team) >> >>\n"
        f"startxref\n{xref_pos}\n%%EOF\n".encode("latin1")
    )
    with open(out_path, "wb") as f:
        f.write(pdf)


def _load_djvulibre():
    """Load libdjvulibre and configure C++ IW44/BZZ symbols needed to emit DjVu."""
    for candidate in ("/usr/lib64/libdjvulibre.so.21", "/usr/lib/x86_64-linux-gnu/libdjvulibre.so.21", "libdjvulibre.so.21"):
        if os.path.exists(candidate) or "/" not in candidate:
            try:
                lib = ctypes.CDLL(candidate)
                break
            except OSError:
                continue
    else:
        raise RuntimeError("libdjvulibre.so.21 not found")

    class IWEncoderParms(ctypes.Structure):
        _fields_ = [("slices", ctypes.c_int), ("bytes", ctypes.c_int), ("decibels", ctypes.c_float)]

    lib._iw_parms_cls = IWEncoderParms
    lib._ZN4DJVU10ByteStream6createEPKvm.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib._ZN4DJVU10ByteStream6createEPKvm.restype = ctypes.c_void_p
    lib._ZN4DJVU10ByteStream6createEv.argtypes = [ctypes.c_void_p]
    lib._ZN4DJVU10ByteStream6createEv.restype = ctypes.c_void_p
    lib._ZNK4DJVU10ByteStream6Memory4tellEv.argtypes = [ctypes.c_void_p]
    lib._ZNK4DJVU10ByteStream6Memory4tellEv.restype = ctypes.c_long
    lib._ZN4DJVU10ByteStream6Memory4seekElib.argtypes = [ctypes.c_void_p, ctypes.c_long, ctypes.c_int, ctypes.c_bool]
    lib._ZN4DJVU10ByteStream6Memory4seekElib.restype = ctypes.c_int
    lib._ZN4DJVU10ByteStream6Memory4readEPvm.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib._ZN4DJVU10ByteStream6Memory4readEPvm.restype = ctypes.c_size_t
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE.restype = ctypes.c_void_p
    lib._ZN4DJVU7GPixmapC1ERNS_10ByteStreamE.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib._ZN4DJVU7GPixmapC1ERNS_10ByteStreamE.restype = None
    lib._ZN4DJVU7GPixmapD1Ev.argtypes = [ctypes.c_void_p]
    lib._ZN4DJVU7GPixmapD1Ev.restype = None
    lib._ZN4DJVU9IW44Image13create_encodeERKNS_7GPixmapENS_2GPINS_7GBitmapEEENS0_8CRCBModeE.argtypes = [
        ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int
    ]
    lib._ZN4DJVU9IW44Image13create_encodeERKNS_7GPixmapENS_2GPINS_7GBitmapEEENS0_8CRCBModeE.restype = ctypes.c_void_p
    lib._ZN4DJVU8IWPixmap6Encode12encode_chunkENS_2GPINS_10ByteStreamEEERKNS_14IWEncoderParmsE.argtypes = [
        ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(IWEncoderParms)
    ]
    lib._ZN4DJVU8IWPixmap6Encode12encode_chunkENS_2GPINS_10ByteStreamEEERKNS_14IWEncoderParmsE.restype = ctypes.c_int
    lib._ZN4DJVU12BSByteStream6createENS_2GPINS_10ByteStreamEEEi.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]
    lib._ZN4DJVU12BSByteStream6createENS_2GPINS_10ByteStreamEEEi.restype = ctypes.c_void_p
    lib._ZN4DJVU12BSByteStream6Encode5writeEPKvm.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    lib._ZN4DJVU12BSByteStream6Encode5writeEPKvm.restype = ctypes.c_size_t
    lib._ZN4DJVU12BSByteStream6Encode5flushEv.argtypes = [ctypes.c_void_p]
    lib._ZN4DJVU12BSByteStream6Encode5flushEv.restype = None
    return lib


def _iff_chunk(fourcc, data):
    pad = b"\x00" if (len(data) & 1) else b""
    return fourcc + struct.pack(">I", len(data)) + data + pad


def _iff_form(form_type, body):
    payload = form_type + body
    pad = b"\x00" if (len(payload) & 1) else b""
    return b"FORM" + struct.pack(">I", len(payload)) + payload + pad


def _read_mem_stream(lib, gp_mem):
    size = lib._ZNK4DJVU10ByteStream6Memory4tellEv(gp_mem[0])
    lib._ZN4DJVU10ByteStream6Memory4seekElib(gp_mem[0], 0, 0, False)
    out_buf = ctypes.create_string_buffer(size)
    got = lib._ZN4DJVU10ByteStream6Memory4readEPvm(gp_mem[0], out_buf, size)
    return out_buf.raw[:got]


def _encode_djvu_page(lib, img, slices_list=(76, 96)):
    """Encode a Pillow RGB Image into a single FORM:DJVU page payload."""
    ppm_buf = io.BytesIO()
    img.save(ppm_buf, format="PPM")
    ppm_bytes = ppm_buf.getvalue()

    in_buf = ctypes.create_string_buffer(ppm_bytes)
    gp_in = (ctypes.c_void_p * 1)(0)
    lib._ZN4DJVU10ByteStream6createEPKvm(ctypes.byref(gp_in), in_buf, len(ppm_bytes))

    pix = ctypes.create_string_buffer(128)
    lib._ZN4DJVU7GPixmapC1ERNS_10ByteStreamE(pix, gp_in[0])

    gp_null_bm = (ctypes.c_void_p * 1)(0)
    gp_iw = (ctypes.c_void_p * 1)(0)
    lib._ZN4DJVU9IW44Image13create_encodeERKNS_7GPixmapENS_2GPINS_7GBitmapEEENS0_8CRCBModeE(
        ctypes.byref(gp_iw), pix, ctypes.byref(gp_null_bm), 2
    )

    bg44_chunks = []
    for sl in slices_list:
        gp_out = (ctypes.c_void_p * 1)(0)
        lib._ZN4DJVU10ByteStream6createEv(ctypes.byref(gp_out))
        gp_arg = (ctypes.c_void_p * 1)(0)
        lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_arg), gp_out[0])
        parms = lib._iw_parms_cls(slices=sl, bytes=0, decibels=0.0)
        res = lib._ZN4DJVU8IWPixmap6Encode12encode_chunkENS_2GPINS_10ByteStreamEEERKNS_14IWEncoderParmsE(
            gp_iw[0], ctypes.byref(gp_arg), ctypes.byref(parms)
        )
        lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_arg), None)
        bg44_chunks.append(_read_mem_stream(lib, gp_out))
        lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_out), None)
        if res == 0:
            break

    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_iw), None)
    lib._ZN4DJVU7GPixmapD1Ev(pix)
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_in), None)

    w, h = img.size
    info_data = struct.pack(">HHBB", w, h, 24, 0) + struct.pack("<H", 100) + struct.pack("BB", 22, 1)
    inner = _iff_chunk(b"INFO", info_data) + b"".join(_iff_chunk(b"BG44", c) for c in bg44_chunks)
    return _iff_form(b"DJVU", inner)


def _bzz_compress(lib, raw_body):
    gp_mem = (ctypes.c_void_p * 1)(0)
    lib._ZN4DJVU10ByteStream6createEv(ctypes.byref(gp_mem))
    gp_arg = (ctypes.c_void_p * 1)(0)
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_arg), gp_mem[0])
    gp_bzz = (ctypes.c_void_p * 1)(0)
    lib._ZN4DJVU12BSByteStream6createENS_2GPINS_10ByteStreamEEEi(ctypes.byref(gp_bzz), ctypes.byref(gp_arg), 50)
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_arg), None)

    raw_buf = ctypes.create_string_buffer(raw_body)
    lib._ZN4DJVU12BSByteStream6Encode5writeEPKvm(gp_bzz[0], raw_buf, len(raw_body))
    lib._ZN4DJVU12BSByteStream6Encode5flushEv(gp_bzz[0])
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_bzz), None)

    compressed = _read_mem_stream(lib, gp_mem)
    lib._ZN4DJVU6GPBase6assignEPNS_9GPEnabledE(ctypes.byref(gp_mem), None)
    return compressed


def build_djvu(out_path):
    """Build a valid 2-page bundled DjVu (cover page + sample content page)."""
    lib = _load_djvulibre()

    cover_img = make_cover_image("DJVU SAMPLE", "WAVELET SCAN", "Format: DJVU (.djvu)", accent=(217, 119, 6))

    page2_img = Image.new("RGB", (360, 480), (249, 245, 235))
    d = ImageDraw.Draw(page2_img)
    d.rectangle([20, 20, 340, 62], fill=(217, 119, 6))
    d.text((34, 35), "LIBRIS DJVU - PAGE 2 / CHAPTER 1", fill=(255, 255, 255))
    d.text((28, 86), "This lightweight multi-page DjVu verifies:", fill=(30, 38, 48))
    d.text((36, 110), "1. Page 1 cover thumbnail extraction.", fill=(50, 62, 74))
    d.text((36, 130), "2. Multi-page IW44 wavelet decoding.", fill=(50, 62, 74))
    d.text((36, 150), "3. Smooth zoom and page navigation.", fill=(50, 62, 74))
    d.rectangle([28, 190, 332, 320], outline=(217, 119, 6), width=2, fill=(242, 246, 244))
    d.rectangle([48, 214, 116, 296], fill=(16, 185, 129))
    d.ellipse([60, 232, 104, 276], fill=(255, 255, 255))
    d.text((134, 226), "DjVuLibre Vector Core", fill=(20, 30, 42))
    d.text((134, 250), "IW44 BG44 + DIRM Ready", fill=(60, 80, 95))
    d.text((134, 274), "BlackBerry 10 Native UI", fill=(217, 119, 6))
    d.text((28, 352), "Tip: Pinch to zoom or swipe to flip between", fill=(70, 80, 90))
    d.text((28, 372), "Page 1 (Cover) and Page 2 (Sample Content).", fill=(70, 80, 90))

    pages = [
        (b"cover.djvu", _encode_djvu_page(lib, cover_img)),
        (b"page2.djvu", _encode_djvu_page(lib, page2_img)),
    ]
    nfiles = len(pages)
    sizes_bytes = b"".join(struct.pack(">I", len(p))[1:] for _, p in pages)
    flags_bytes = bytes([0x01] * nfiles)
    ids_bytes = b"".join(name + b"\x00" for name, _ in pages)
    tail_bzz = _bzz_compress(lib, sizes_bytes + flags_bytes + ids_bytes)

    placeholder = _iff_chunk(b"DIRM", struct.pack(">BH", 0x81, nfiles) + b"\x00" * (4 * nfiles) + tail_bzz)
    cur = 4 + 12 + len(placeholder)
    offsets = []
    for _, pform in pages:
        offsets.append(cur)
        cur += len(pform) + (len(pform) & 1)

    dirm_chunk = _iff_chunk(
        b"DIRM",
        struct.pack(">BH", 0x81, nfiles) + b"".join(struct.pack(">I", o) for o in offsets) + tail_bzz,
    )
    all_pages = b"".join(p + (b"\x00" if len(p) & 1 else b"") for _, p in pages)
    with open(out_path, "wb") as f:
        f.write(b"AT&T" + _iff_form(b"DJVM", dirm_chunk + all_pages))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    build_epub(os.path.join(OUT_DIR, "Libris_Sample_EPUB.epub"))
    build_fb2(os.path.join(OUT_DIR, "Libris_Sample_FB2.fb2"))
    build_cbz(os.path.join(OUT_DIR, "Libris_Sample_CBZ.cbz"))
    build_txt(os.path.join(OUT_DIR, "Libris_Sample_TXT.txt"))
    build_mobi(os.path.join(OUT_DIR, "Libris_Sample_MOBI.mobi"))
    build_pdf(os.path.join(OUT_DIR, "Libris_Sample_PDF.pdf"))
    build_djvu(os.path.join(OUT_DIR, "Libris_Sample_DJVU.djvu"))
    for fn in sorted(os.listdir(OUT_DIR)):
        print(fn, os.path.getsize(os.path.join(OUT_DIR, fn)), "bytes")


if __name__ == "__main__":
    main()
