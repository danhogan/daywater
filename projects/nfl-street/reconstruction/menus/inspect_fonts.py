"""Preview original FNTG atlas passes and checked glyph placement privately on W:."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
from inspect_assets import Texture, png


class Font(ctypes.Structure):
    _fields_ = [('bytes', ctypes.c_void_p), ('size', ctypes.c_size_t),
                ('glyphs', ctypes.c_size_t), ('kerning', ctypes.c_size_t),
                ('atlas', ctypes.c_size_t), ('count', ctypes.c_uint),
                ('width', ctypes.c_uint), ('height', ctypes.c_uint),
                ('line_height', ctypes.c_uint)]


class Box(ctypes.Structure):
    _fields_ = [('x', ctypes.c_int32), ('y', ctypes.c_int32),
                ('width', ctypes.c_uint32), ('height', ctypes.c_uint32),
                ('u', ctypes.c_uint16), ('v', ctypes.c_uint16)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fonts', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.relative_to(Path('/mnt/w/Daywater').resolve())
    output.mkdir(parents=True, exist_ok=True)
    root = Path(__file__).resolve().parent
    library = output / 'font-preview.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall',
                    '-Wextra', '-Werror', str(root / 'font.c'),
                    str(root.parent / 'native_assets/texture.c'),
                    '-o', str(library)], check=True)
    lib = ctypes.CDLL(str(library))
    lib.nflstreet_font_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                       ctypes.POINTER(Font)]
    lib.nflstreet_font_atlas.argtypes = [ctypes.POINTER(Font), ctypes.c_uint,
                                        ctypes.POINTER(Texture)]
    lib.nflstreet_font_layout.argtypes = [ctypes.POINTER(Font), ctypes.c_char_p,
        ctypes.c_uint, ctypes.c_double, ctypes.POINTER(Box), ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint)]
    lib.nflstreet_texture_close.argtypes = [ctypes.POINTER(Texture)]
    records = []
    for index in range(4):
        data = (args.fonts / f'{index:05d}.bin').read_bytes()
        buffer = ctypes.create_string_buffer(data)
        font = Font()
        if not lib.nflstreet_font_open(buffer, len(data), ctypes.byref(font)):
            raise RuntimeError(f'Font {index} failed to open')
        boxes, count = (Box * 354)(), ctypes.c_uint()
        if not lib.nflstreet_font_layout(ctypes.byref(font), b'Press START', 1,
                0, boxes, 354, ctypes.byref(count)):
            raise RuntimeError('Glyph layout failed')
        glyphs = boxes[:count.value]
        xmin = min(g.x for g in glyphs)
        ymin = min(g.y for g in glyphs)
        width = max(g.x + g.width for g in glyphs) - xmin
        height = max(g.y + g.height for g in glyphs) - ymin
        for stage in range(2):
            atlas = Texture()
            if not lib.nflstreet_font_atlas(ctypes.byref(font), stage,
                                            ctypes.byref(atlas)):
                raise RuntimeError(f'Atlas {index}/{stage} failed')
            try:
                name = f'font-{index}-pass-{stage}.png'
                encoded = png(atlas)
                (output / name).write_bytes(encoded)
                pixels = (ctypes.c_uint32 * (width * height))()
                for g in glyphs:
                    for y in range(g.height):
                        for x in range(g.width):
                            pixels[(g.y - ymin + y) * width + g.x - xmin + x] = (
                                atlas.bgra[(g.v + y) * atlas.width + g.u + x])
                strip = png(Texture(width, height, pixels))
                strip_name = f'font-{index}-press-start-pass-{stage}.png'
                (output / strip_name).write_bytes(strip)
                records.append(dict(font=index, pass_index=stage,
                    source_sha256=hashlib.sha256(data).hexdigest(),
                    atlas=name, atlas_sha256=hashlib.sha256(encoded).hexdigest(),
                    glyph_strip=strip_name,
                    glyph_strip_sha256=hashlib.sha256(strip).hexdigest()))
            finally:
                lib.nflstreet_texture_close(ctypes.byref(atlas))
        sentinel = Texture(123, 456, None)
        if lib.nflstreet_font_atlas(ctypes.byref(font), 2, ctypes.byref(sentinel)):
            raise RuntimeError('Invalid palette pass accepted')
        truncated = Font.from_buffer_copy(font)
        truncated.size = font.atlas + 16
        if lib.nflstreet_font_atlas(ctypes.byref(truncated), 0,
                                    ctypes.byref(sentinel)):
            raise RuntimeError('Truncated atlas accepted')
        if (sentinel.width, sentinel.height, bool(sentinel.bgra)) != (123, 456, False):
            raise RuntimeError('Rejected atlas changed output')
    report = dict(records=records, rejected_atlas_guards=8,
        limitation='Atlas/glyph diagnostics only. Original screen transform, colors, '
                   'filtering, compositing, timing and navigation are unfinished.')
    (output / 'font-previews.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(records)} atlas passes and glyph strips; 8 atomic guards passed')


if __name__ == '__main__':
    main()
