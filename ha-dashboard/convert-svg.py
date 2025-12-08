"""
SVG converter for ha-floorplan
Replaces foreignObject with native SVG text elements
"""

import xml.etree.ElementTree as ET
import sys, re, argparse
from typing import Optional, Tuple
from pathlib import Path

# Parse a numeric value (with optional decimal or sign) from a string.
def parse_number(value: str, default: float = 0.0) -> float:
    if not value: return default
    match = re.match(r'[-+]?(?:\d*\.)?\d+', str(value))
    return float(match.group()) if match else default

# Extract a CSS property’s value from a style attribute string.
def extract_style(prop: str, style: str) -> Optional[str]:
    if not style: return None
    match = re.search(rf'{re.escape(prop)}\s*:\s*([^;]+)', style)
    return match.group(1).strip() if match else None

# Traverse up element tree to find nearest ancestor with a `data-cell-id`.
def find_element_id(element: ET.Element, parent_map: dict) -> Optional[str]:
    current = element
    while current is not None:
        if cell_id := current.get('data-cell-id'):
            return cell_id
        current = parent_map.get(id(current))
    return None

# Compute position and style info from a <foreignObject> subtree.
def get_text_position_and_style(foreign: ET.Element) -> Tuple[float, float, str, float, str, str, str]:
    # Default parameters
    margin_left = padding_top = width = 0.0
    font_size = '14px'; font_size_num = 14.0
    fill_color = '#000000'; font_family = 'Helvetica, Arial, sans-serif'; font_weight = 'bold'
    
    # Inspect nested elements for styled attributes
    for elem in foreign.iter():
        style = elem.get('style', '')
        if not style: continue
        if ml := extract_style('margin-left', style): margin_left = parse_number(ml)
        if pt := extract_style('padding-top', style): padding_top = parse_number(pt)
        if w := extract_style('width', style): width = parse_number(w)
        if fs := extract_style('font-size', style):
            if fs not in ('0', '0px'): font_size = fs; font_size_num = parse_number(fs, 14.0)
        if color := extract_style('color', style): fill_color = color
        if family := extract_style('font-family', style): font_family = family
        if weight := extract_style('font-weight', style): font_weight = weight
    
    # Return computed text anchor and visual style properties
    return margin_left + (width / 2), padding_top + (font_size_num * 0.35), font_size, font_size_num, fill_color, font_family, font_weight

# Convert draw.io SVGs (with foreignObject) to standard SVG text elements
def convert_svg(input_path: Path, output_path: Optional[Path] = None) -> bool:
    # Validate input file
    if not input_path.exists(): print(f"❌ Error: '{input_path}' not found!"); return False
    if input_path.suffix.lower() != '.svg': print("❌ Error: Input must be .svg file"); return False
    
    output_path = output_path or input_path.with_suffix('.converted.svg')
    if output_path.exists(): print(f"⚠️ Warning: Overwriting '{output_path}'")
    
    # Safely parse XML tree
    try: tree = ET.parse(input_path)
    except ET.ParseError as e: print(f"❌ Error: Invalid SVG: {e}"); return False
    except OSError as e: print(f"❌ Error: Cannot read '{input_path}': {e}"); return False
    
    root = tree.getroot()
    # Detect XML namespace
    m = re.match(r'\{([^}]+)\}svg', root.tag)
    ns = m.group(1) if m else 'http://www.w3.org/2000/svg'
    FOREIGN_OBJECT = f'{{{ns}}}foreignObject'; TEXT = f'{{{ns}}}text'
    
    ET.register_namespace('', ns); ET.register_namespace('xlink', 'http://www.w3.org/1999/xlink')
    print(f"🔄 Processing {input_path}...")
    
    # Build lookup of each element’s parent for reverse traversal
    parent_map = {}
    try:
        for parent in root.iter():
            for child in parent: parent_map[id(child)] = parent
    except Exception as e: print(f"❌ Error: Cannot build tree: {e}"); return False
    
    converted = removed_empty = 0
    # Iterate all <foreignObject> elements
    for foreign in list(root.iter(FOREIGN_OBJECT)):
        text_content = ''.join(foreign.itertext()).strip()
        if not text_content:
            # Remove empty containers with no visible text
            if parent := parent_map.get(id(foreign)): parent.remove(foreign); removed_empty += 1
            continue
        
        elem_id = find_element_id(foreign, parent_map) or f'element_{converted}'
        if not elem_id.startswith('element_'): print(f" ⚠️ No data-cell-id for '{text_content[:30]}...', using {elem_id}")
        
        # Compute position/style; continue on failure
        try:
            x, y, font_size, font_size_num, fill_color, font_family, font_weight = get_text_position_and_style(foreign)
            if x == 0 or y == 0: print(f" ⚠️ Zero coord '{text_content[:20]}...' (x={x:.1f}, y={y:.1f})")
        except Exception as e:
            print(f" ❌ Error processing '{text_content[:30]}...': {e}"); continue
        
        # Create replacement <text> element with matching attributes
        text_elem = ET.Element(TEXT)
        text_elem.attrib.update({
            'id': elem_id, 'x': str(round(x, 2)), 'y': str(round(y, 2)),
            'font-size': font_size, 'fill': fill_color, 'font-family': font_family,
            'text-anchor': 'middle', 'font-weight': font_weight
        })
        
        # Add tspans for multi-line text; plain text otherwise
        lines = text_content.splitlines()
        if len(lines) > 1:
            for i, line in enumerate(lines):
                tspan = ET.SubElement(text_elem, f'{{{ns}}}tspan', {
                    'x': text_elem.attrib['x'], 'dy': str(font_size_num * (1.2 if i else 0))
                })
                tspan.text = line
        else:
            text_elem.text = text_content
        
        # Replace foreignObject node in-place with new text element
        if parent := parent_map.get(id(foreign)):
            try:
                idx = list(parent).index(foreign)
                parent.remove(foreign); parent.insert(idx, text_elem)
                print(f" ✓ {elem_id}: '{text_content}' at ({x:.1f}, {y:.1f})")
                converted += 1
            except (ValueError, IndexError) as e:
                print(f" ❌ Failed replace '{text_content[:20]}...': {e}")
        else:
            print(f" ⚠️ Orphaned '{text_content[:20]}...'")
    
    # Write back modified SVG
    try:
        tree.write(output_path, encoding='utf-8', xml_declaration=True)
        print(f"\n✅ Success! Converted: {converted} texts", end='')
        if removed_empty: print(f" | Removed: {removed_empty} empties")
        print(f" → {output_path}")
        return True
    except OSError as e: print(f"❌ Error writing '{output_path}': {e}"); return False

# CLI entry point
def main():
    parser = argparse.ArgumentParser(description="draw.io → ha-floorplan SVG converter",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n  python convert-svg.py floorplan.svg\n  python convert-svg.py floorplan.svg -o ha-floorplan.svg")
    parser.add_argument('input', nargs='?', help="Input SVG")
    parser.add_argument('-o', '--output', help="Output SVG (optional)")
    args = parser.parse_args()
    
    if not args.input: parser.print_help(); sys.exit(1)
    sys.exit(0 if convert_svg(Path(args.input), Path(args.output) if args.output else None) else 1)

# Run module directly
if __name__ == "__main__": main()