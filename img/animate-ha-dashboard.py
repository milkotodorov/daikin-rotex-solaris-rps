#!/usr/bin/env python3
"""
Inject CSS blink animations into ha-dashboard.svg after draw.io re-export.

Animated elements:
  - ha_txt    (H letter — foreignObject)
  - err_val   (error letter — foreignObject)
  - ts_val    (storage temp value — foreignObject)
  - p1_on_led (green LED — ellipse via parent g, via CSS keyframes)
  - p2_on_led (green LED — ellipse via parent g, via CSS keyframes)

Usage:
  python3 animate-ha-dashboard.py [path/to/ha-dashboard.svg]
  (defaults to ha-dashboard.svg in the same folder as this script)
"""

import re
import sys
import argparse
from pathlib import Path

SVG_PATH = Path(__file__).parent / "ha-dashboard.svg"

KEYFRAMES = """
@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0; }
}
#cell-p1_on_led, #cell-p2_on_led {
  animation: blink 1.5s ease-in-out infinite;
}"""

ANIM_STYLE = "animation: blink 1.5s ease-in-out infinite;"

FO_OLD = 'overflow: visible; text-align: left;'
FO_NEW = f'overflow: visible; text-align: left; {ANIM_STYLE}'


def find_cell_bounds(content: str, cell_id: str) -> tuple[int, int] | None:
    """Return (start, end) indices of the <g id="cell-{cid}"> group, or None."""
    start = content.find(f'id="{cell_id}"')
    if start == -1:
        return None
    # Walk back to the opening < of the <g> tag
    tag_start = content.rfind('<', 0, start)
    if tag_start == -1:
        return None
    # Count nested <g> tags to find the matching </g>
    depth = 0
    pos = tag_start
    while pos < len(content):
        open_g = content.find('<g', pos)
        close_g = content.find('</g>', pos)
        if open_g == -1:
            open_g = len(content)
        if close_g == -1:
            break
        if open_g < close_g:
            depth += 1
            pos = open_g + 2
        else:
            depth -= 1
            pos = close_g + 4
            if depth == 0:
                return tag_start, pos
    return None


def inject(content: str) -> tuple[str, int, int]:
    injected = 0
    warnings = 0

    # 1. Append keyframes into existing <style> block
    style_m = re.search(r'<style>(.*?)</style>', content, re.DOTALL)
    if not style_m:
        print("❌ Error: no <style> block found in SVG", file=sys.stderr)
        sys.exit(1)
    if '@keyframes blink' in style_m.group():
        print(" ✓ keyframes: already present — skipping")
    else:
        old_style = style_m.group()
        new_style = old_style[:-8] + KEYFRAMES + "\n</style>"
        content = content.replace(old_style, new_style, 1)
        print(" ✓ keyframes: @keyframes blink + #cell-p1_on_led, #cell-p2_on_led injected into <style>")
        injected += 1

    # 2. Animate foreignObject — search only within the cell's own <g> group
    for cid in ["ha_txt", "err_val", "ts_val"]:
        bounds = find_cell_bounds(content, f"cell-{cid}")
        if bounds is None:
            print(f" ⚠️ Warning: cell-{cid} not found in SVG — skipping")
            warnings += 1
            continue
        cell_start, cell_end = bounds
        cell_content = content[cell_start:cell_end]

        if FO_NEW in cell_content:
            print(f" ✓ {cid}: already animated — skipping")
            continue

        fo_pos = cell_content.find(f'<foreignObject style="{FO_OLD}"')
        if fo_pos == -1:
            print(f" ⚠️ Warning: foreignObject for cell-{cid} not found — style mismatch?")
            warnings += 1
            continue

        new_cell = cell_content[:fo_pos] + f'<foreignObject style="{FO_NEW}"' + cell_content[fo_pos + len(f'<foreignObject style="{FO_OLD}"'):]
        content = content[:cell_start] + new_cell + content[cell_end:]
        print(f" ✓ {cid}: foreignObject animation injected (blink 1.5s ease-in-out infinite)")
        injected += 1

    return content, injected, warnings


def main():
    parser = argparse.ArgumentParser(
        description="Inject CSS blink animations into ha-dashboard.svg after draw.io re-export",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n  python3 animate-ha-dashboard.py\n  python3 animate-ha-dashboard.py path/to/ha-dashboard.svg",
    )
    parser.add_argument('input', nargs='?', help="Input SVG (default: ha-dashboard.svg next to this script)")
    args = parser.parse_args()

    path = Path(args.input) if args.input else SVG_PATH
    if not path.exists():
        print(f"❌ Error: '{path}' not found", file=sys.stderr)
        sys.exit(1)
    if path.suffix.lower() != '.svg':
        print(f"❌ Error: Input must be an .svg file", file=sys.stderr)
        sys.exit(1)

    print(f"🔄 Processing {path}...")
    try:
        content = path.read_text(encoding="utf-8")
    except OSError as e:
        print(f"❌ Error: Cannot read '{path}': {e}", file=sys.stderr)
        sys.exit(1)

    content, injected, warnings = inject(content)

    try:
        path.write_text(content, encoding="utf-8")
    except OSError as e:
        print(f"❌ Error: Cannot write '{path}': {e}", file=sys.stderr)
        sys.exit(1)

    summary = f"\n✅ Done! Injected: {injected} animations"
    if warnings:
        summary += f" | Warnings: {warnings}"
    print(summary + f" → {path}")


if __name__ == "__main__":
    main()
