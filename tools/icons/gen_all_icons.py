from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple


SYMBOL_RE = re.compile(r"const\s+unsigned\s+char\s+([A-Za-z_]\w*)\s*\[")


def list_numeric_codes(folder: Path) -> List[str]:
    return sorted((p.stem for p in folder.glob("*.c") if p.stem.isdigit()), key=lambda s: int(s))


def detect_symbol(file_path: Path, fallback_symbol: str) -> str:
    content = file_path.read_text(encoding="utf-8", errors="ignore")
    m = SYMBOL_RE.search(content)
    return m.group(1) if m else fallback_symbol


def build_header(
    target_header: Path,
    include_subdir: str,
    codes: List[str],
    entry_type: str,
    map_name: str,
    count_name: str,
    getter_name: str,
    alias_suffix: str,
) -> None:
    source_dir = target_header.parent / include_subdir
    symbol_by_code: Dict[str, str] = {}
    lines: List[str] = []

    guard = target_header.stem.upper() + "_H"
    lines += [f"#ifndef {guard}", f"#define {guard}", ""]
    lines += ["#include <stddef.h>", "#include <Arduino.h>", ""]
    lines += ["typedef struct {", "  const char* code;", "  const unsigned char* bmp;", f"}} {entry_type};", ""]

    for code in codes:
        src_file = source_dir / f"{code}.c"
        fallback = f"gImage_{code}_{alias_suffix}"
        symbol = detect_symbol(src_file, fallback)
        symbol_by_code[code] = symbol
        alias = fallback
        lines += [f"#define {symbol} {alias}", f"#include \"{include_subdir}/{code}.c\"", f"#undef {symbol}"]

    lines += [""]
    lines += [f"static const {entry_type} {map_name}[] = {{"]
    for code in codes:
        lines += [f"  {{ \"{code}\", gImage_{code}_{alias_suffix} }},"]
    lines += ["};", ""]
    lines += [f"static const size_t {count_name} = sizeof({map_name}) / sizeof({map_name}[0]);", ""]
    lines += [f"static inline const unsigned char* {getter_name}(const String& code) {{"]
    lines += [f"  for (size_t i = 0; i < {count_name}; i++) {{"]
    lines += [f"    if (code == {map_name}[i].code) {{", f"      return {map_name}[i].bmp;", "    }", "  }", "  return NULL;", "}"]
    lines += ["", "#endif", ""]

    target_header.write_text("\n".join(lines), encoding="utf-8")


def compare_sets(m_codes: List[str], s_codes: List[str]) -> Tuple[List[str], List[str]]:
    m_set = set(m_codes)
    s_set = set(s_codes)
    only_m = sorted(m_set - s_set, key=lambda s: int(s))
    only_s = sorted(s_set - m_set, key=lambda s: int(s))
    return only_m, only_s


def normalize_mqtt_icon_code(code: str) -> str:
    value = code.strip()
    lower = value.lower()
    if lower.endswith("-fill"):
        value = value[:-5]
    return value


def load_contract_codes(contract_file: Path) -> List[str]:
    if not contract_file.exists():
        return []

    codes: Set[str] = set()
    for raw in contract_file.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        normalized = normalize_mqtt_icon_code(line)
        if normalized.isdigit():
            codes.add(normalized)
    return sorted(codes, key=lambda s: int(s))


def list_bmp_codes(bmp_dir: Path) -> List[str]:
    if not bmp_dir.exists():
        return []
    codes: Set[str] = set()
    for p in bmp_dir.glob("*.bmp"):
        stem = p.stem.strip()
        normalized = normalize_mqtt_icon_code(stem)
        if normalized.isdigit():
            codes.add(normalized)
    return sorted(codes, key=lambda s: int(s))


def diff_codes(expected: List[str], actual: List[str]) -> List[str]:
    exp = set(expected)
    act = set(actual)
    return sorted(exp - act, key=lambda s: int(s))


def compile_project(project_root: Path) -> int:
    cmd = [
        "arduino-cli",
        "compile",
        "--fqbn",
        "esp32:esp32:esp32:PartitionScheme=huge_app",
        "--output-dir",
        "./build",
        "./e_weather/e_weather.ino",
    ]
    print("[compile] " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=project_root)
    return proc.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate icon_m.h and icon_s.h from icon folders.")
    parser.add_argument(
        "--project-root",
        default=str(Path(__file__).resolve().parents[2]),
        help="Project root directory (default: auto-detected).",
    )
    parser.add_argument("--compile", action="store_true", help="Compile project after generating headers.")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Fail when codes in 7272 and 3636 are inconsistent.",
    )
    parser.add_argument(
        "--contract-file",
        default="tools/icons/mqtt_icon_codes.txt",
        help="Relative path from project root for expected MQTT icon code list.",
    )
    parser.add_argument(
        "--bmp-dir",
        default="e_weather/icons",
        help="Relative path from project root for BMP fallback folder.",
    )
    args = parser.parse_args()

    project_root = Path(args.project_root).resolve()
    icon_dir = project_root / "e_weather" / "icon"
    medium_dir = icon_dir / "7272"
    small_dir = icon_dir / "3636"

    if not medium_dir.exists() or not small_dir.exists():
        print(f"[error] icon folders not found under: {icon_dir}")
        return 2

    m_codes = list_numeric_codes(medium_dir)
    s_codes = list_numeric_codes(small_dir)

    if not m_codes:
        print("[error] no numeric *.c files in 7272/")
        return 2
    if not s_codes:
        print("[error] no numeric *.c files in 3636/")
        return 2

    build_header(
        target_header=icon_dir / "icon_m.h",
        include_subdir="7272",
        codes=m_codes,
        entry_type="WeatherIconMapEntryM",
        map_name="WEATHER_ICON_MAP_M",
        count_name="WEATHER_ICON_MAP_M_COUNT",
        getter_name="getMediumIconData",
        alias_suffix="M",
    )
    build_header(
        target_header=icon_dir / "icon_s.h",
        include_subdir="3636",
        codes=s_codes,
        entry_type="WeatherIconMapEntryS",
        map_name="WEATHER_ICON_MAP_S",
        count_name="WEATHER_ICON_MAP_S_COUNT",
        getter_name="getSmallIconData",
        alias_suffix="S",
    )

    print(f"[ok] generated: {icon_dir / 'icon_m.h'} ({len(m_codes)} entries)")
    print(f"[ok] generated: {icon_dir / 'icon_s.h'} ({len(s_codes)} entries)")

    only_m, only_s = compare_sets(m_codes, s_codes)
    if only_m:
        print("[warn] codes only in 7272:", ", ".join(only_m))
    if only_s:
        print("[warn] codes only in 3636:", ", ".join(only_s))
    if args.strict and (only_m or only_s):
        print("[error] strict mode failed due to code mismatch between 7272 and 3636")
        return 3

    # Contract validation: MQTT code -> array coverage -> BMP fallback coverage
    contract_file = project_root / args.contract_file
    contract_codes = load_contract_codes(contract_file)
    bmp_dir = project_root / args.bmp_dir
    bmp_codes = list_bmp_codes(bmp_dir)
    if contract_codes:
        missing_m = diff_codes(contract_codes, m_codes)
        missing_s = diff_codes(contract_codes, s_codes)
        missing_bmp = diff_codes(contract_codes, bmp_codes) if bmp_dir.exists() else contract_codes

        print(f"[contract] source: {contract_file}")
        print(f"[contract] expected MQTT codes: {len(contract_codes)}")
        print(f"[contract] covered by 7272 arrays: {len(contract_codes) - len(missing_m)}/{len(contract_codes)}")
        print(f"[contract] covered by 3636 arrays: {len(contract_codes) - len(missing_s)}/{len(contract_codes)}")
        if bmp_dir.exists():
            print(f"[contract] covered by BMP fallback: {len(contract_codes) - len(missing_bmp)}/{len(contract_codes)} ({bmp_dir})")
        else:
            print(f"[contract] BMP fallback dir not found: {bmp_dir}")

        if missing_m:
            print("[contract][missing] 7272:", ", ".join(missing_m))
        if missing_s:
            print("[contract][missing] 3636:", ", ".join(missing_s))
        if missing_bmp:
            print("[contract][missing] bmp:", ", ".join(missing_bmp))

        if args.strict and (missing_m or missing_s or missing_bmp):
            print("[error] strict mode failed due to contract coverage gaps")
            return 4
    else:
        print(f"[contract][warn] no contract codes loaded (missing or empty): {contract_file}")

    if args.compile:
        return compile_project(project_root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
