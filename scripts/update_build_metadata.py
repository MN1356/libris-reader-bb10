#!/usr/bin/env python3
"""Synchronize BuildInfo.hpp and changelogs with bar-descriptor.xml."""
import argparse
import os
import re
import time
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


def prepend_changelog_entry(path, header_prefix, version, build_num, note, merge_into_top=False):
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

    if merge_into_top and existing.strip():
        lines = existing.lstrip("\n").splitlines()
        if lines and lines[0].startswith(f"{header_prefix} "):
            lines[0] = f"{header_prefix} {version} (Build {build_num}) - {today}:"
            is_generic = any("tối ưu ổn định và đồng bộ hiển thị" in b or "stability improvements and build sync" in b for b in formatted_bullets)
            if not is_generic:
                insert_at = 1
                while insert_at < len(lines) and lines[insert_at].startswith("-"):
                    insert_at += 1
                for b in formatted_bullets:
                    if b not in lines[:insert_at]:
                        lines.insert(insert_at, b)
                        insert_at += 1
            with open(path, "w", encoding="utf-8") as f:
                f.write("\n".join(lines) + "\n")
            return

    block = f"{header_prefix} {version} (Build {build_num}) - {today}:\n" + "\n".join(formatted_bullets) + "\n\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(block + existing.lstrip("\n"))


def bump_descriptor_version(xml_path, bump_type):
    text = open(xml_path, "r", encoding="utf-8").read()
    m_ver = re.search(r"<versionNumber>([^<]+)</versionNumber>", text)
    old_ver = m_ver.group(1).strip() if m_ver else "2.0.0"
    parts = [int(p) for p in re.findall(r"\d+", old_ver)[:3]]
    while len(parts) < 3:
        parts.append(0)
    major, minor, patch = parts[0], parts[1], parts[2]
    if bump_type == "major":
        major += 1
        minor = 0
        patch = 0
    elif bump_type == "minor":
        minor += 1
        patch = 0
    elif bump_type == "patch":
        patch += 1
    new_ver = f"{major}.{minor}.{patch}"
    if new_ver != old_ver:
        updated = re.sub(
            r"<versionNumber>[^<]+</versionNumber>",
            f"<versionNumber>{new_ver}</versionNumber>",
            text,
            count=1,
        )
        with open(xml_path, "w", encoding="utf-8") as f:
            f.write(updated)
        print(f"--> Auto-incremented versionNumber: {old_ver} -> {new_ver}")
    return new_ver


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-dir", required=True)
    parser.add_argument("--bumped", action="store_true")
    parser.add_argument("--bump-version", choices=["none", "patch", "minor", "major"], default="none")
    args = parser.parse_args()

    xml_path = os.path.join(args.project_dir, "bar-descriptor.xml")
    ts_path = os.path.join(args.project_dir, ".last_build_ts")
    merge_window_sec = int(os.environ.get("MERGE_WINDOW_MINUTES", "60")) * 60
    now_ts = time.time()
    within_merge_window = False
    if os.path.exists(ts_path):
        try:
            last_ts = float(open(ts_path, "r", encoding="utf-8").read().strip())
            within_merge_window = (now_ts - last_ts) < merge_window_sec
        except Exception:
            within_merge_window = False

    if args.bump_version != "none" and not (within_merge_window and args.bump_version == "patch"):
        bump_descriptor_version(xml_path, args.bump_version)
    elif within_merge_window and args.bump_version == "patch":
        print("--> Build within 1h merge window: keeping current versionNumber & merging changelog.")

    with open(ts_path, "w", encoding="utf-8") as f:
        f.write(str(int(now_ts)))

    ver, bld = read_descriptor(xml_path)
    write_build_info_header(os.path.join(args.project_dir, "src", "BuildInfo.hpp"), ver, bld)

    if args.bumped:
        note_vi = os.environ.get("CHANGELOG_VI", f"- Cập nhật bản dựng Build {bld}: tối ưu ổn định và đồng bộ hiển thị.")
        note_en = os.environ.get("CHANGELOG_EN", f"- Updated to Build {bld}: stability improvements and build sync.")
        prepend_changelog_entry(os.path.join(args.project_dir, "assets", "changelog_vi.txt"), "Phiên bản", ver, bld, note_vi, merge_into_top=within_merge_window)
        prepend_changelog_entry(os.path.join(args.project_dir, "assets", "changelog_en.txt"), "Version", ver, bld, note_en, merge_into_top=within_merge_window)


if __name__ == "__main__":
    main()
