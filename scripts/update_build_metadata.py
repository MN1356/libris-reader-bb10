#!/usr/bin/env python3
"""Synchronize BuildInfo.hpp and changelogs with bar-descriptor.xml."""
import argparse
import os
import re
from datetime import datetime


def read_descriptor(xml_path):
    text = open(xml_path, "r", encoding="utf-8").read()
    m_ver = re.search(r"<versionNumber>([^<]+)</versionNumber>", text)
    m_bld = re.search(r"<buildId>([^<]+)</buildId>", text)
    ver = m_ver.group(1).strip() if m_ver else "2.0.0"
    bld = m_bld.group(1).strip() if m_bld else "200"
    return ver, bld


def write_build_info_header(header_path, version, build_num):
    content = (
        "#ifndef BUILDINFO_HPP\n"
        "#define BUILDINFO_HPP\n\n"
        f'#define LIBRIS_APP_VERSION "{version}"\n'
        f'#define LIBRIS_BUILD_NUMBER "{build_num}"\n\n'
        "#endif // BUILDINFO_HPP\n"
    )
    if os.path.exists(header_path):
        old = open(header_path, "r", encoding="utf-8").read()
        if old == content:
            return
    with open(header_path, "w", encoding="utf-8") as f:
        f.write(content)


def prepend_changelog_entry(path, header_prefix, version, build_num, note):
    today = datetime.now().strftime("%Y-%m-%d")
    marker = f"(Build {build_num})"
    existing = ""
    if os.path.exists(path):
        existing = open(path, "r", encoding="utf-8").read()
    if marker in existing:
        return
    note_lines = [ln.strip() for ln in note.strip().splitlines() if ln.strip()]
    formatted_bullets = []
    for ln in note_lines:
        formatted_bullets.append(ln if ln.startswith("-") else f"- {ln}")
    if not formatted_bullets:
        formatted_bullets = [f"- Build {build_num}."]
    block = f"{header_prefix} {version} (Build {build_num}) - {today}:\n" + "\n".join(formatted_bullets) + "\n\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(block + existing.lstrip("\n"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-dir", required=True)
    parser.add_argument("--bumped", action="store_true")
    args = parser.parse_args()

    xml_path = os.path.join(args.project_dir, "bar-descriptor.xml")
    ver, bld = read_descriptor(xml_path)
    write_build_info_header(os.path.join(args.project_dir, "src", "BuildInfo.hpp"), ver, bld)

    if args.bumped:
        note_vi = os.environ.get("CHANGELOG_VI", f"- Cập nhật bản dựng Build {bld}: tối ưu ổn định và đồng bộ hiển thị.")
        note_en = os.environ.get("CHANGELOG_EN", f"- Updated to Build {bld}: stability improvements and build sync.")
        prepend_changelog_entry(os.path.join(args.project_dir, "assets", "changelog_vi.txt"), "Phiên bản", ver, bld, note_vi)
        prepend_changelog_entry(os.path.join(args.project_dir, "assets", "changelog_en.txt"), "Version", ver, bld, note_en)


if __name__ == "__main__":
    main()
