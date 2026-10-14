#!/usr/bin/env python3
"""Optional: python3 tools/subset_font.py /path/to/upstream/font/yuanli.ttf
Requires fonttools. Keep original font licensing and attribution.
"""
import sys
from pathlib import Path
from fontTools import subset
root = Path(__file__).resolve().parents[1]
chars = set('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 /:.,·←→↑↓−+-【】（）')
for file in list((root/'source').glob('*.cpp')) + list((root/'include').glob('*.hpp')):
    chars.update(file.read_text())
options = subset.Options()
options.notdef_glyph = True
options.notdef_outline = True
options.recalc_bounds = True
font = subset.load_font(sys.argv[1], options)
worker = subset.Subsetter(options=options)
worker.populate(text=''.join(chars))
worker.subset(font)
subset.save_font(font, str(root/'romfs/font.ttf'), options)
print('Wrote', root/'romfs/font.ttf')
