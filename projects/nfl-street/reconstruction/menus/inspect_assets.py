"""Inventory original UI packages and preview MMAP images privately on W:."""
import argparse
import ctypes
import hashlib
import html
import json
from pathlib import Path
import struct
import subprocess
import zlib


class Texture(ctypes.Structure):
    _fields_ = [('width', ctypes.c_uint), ('height', ctypes.c_uint),
                ('bgra', ctypes.POINTER(ctypes.c_uint32))]


def chunk(tag, data):
    return (struct.pack('>I', len(data)) + tag + data
            + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))


def png(texture):
    rows = []
    for y in range(texture.height):
        row = bytearray([0])
        for x in range(texture.width):
            pixel = texture.bgra[y * texture.width + x]
            row.extend(((pixel >> 16) & 255, (pixel >> 8) & 255,
                        pixel & 255, pixel >> 24))
        rows.append(row)
    header = struct.pack('>IIBBBBB', texture.width, texture.height, 8, 6, 0, 0, 0)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', header)
            + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unpacked', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.relative_to(Path('/mnt/w/Daywater').resolve())
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent.parent / 'native_assets/texture.c'
    library = output / 'texture-preview.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra',
                    str(source), '-o', str(library)], check=True)
    decoder = ctypes.CDLL(str(library))
    decoder.nflstreet_texture_open_image.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
        ctypes.c_uint, ctypes.c_uint, ctypes.POINTER(Texture)]
    decoder.nflstreet_texture_open_image.restype = ctypes.c_int
    decoder.nflstreet_texture_close.argtypes = [ctypes.POINTER(Texture)]
    records, figures = [], []
    for folder in sorted(args.unpacked.glob('uis_*')):
        for path in sorted(folder.glob('*.bin')):
            data = path.read_bytes()
            record = dict(package=folder.name, entry=path.stem, bytes=len(data),
                          magic=data[:4].decode('ascii', errors='replace'),
                          sha256=hashlib.sha256(data).hexdigest(), images=[])
            records.append(record)
            if data[:4] != b'MMAP' or len(data) < 40:
                continue
            images = struct.unpack_from('>H', data, 14)[0]
            buffer = ctypes.create_string_buffer(data)
            for image in range(images):
                texture = Texture()
                if not decoder.nflstreet_texture_open_image(buffer, len(data), image,
                                                           0, ctypes.byref(texture)):
                    record['images'].append(dict(image=image, decoded=False))
                    continue
                try:
                    name = f'{folder.name}-{path.stem}-{image}.png'
                    encoded = png(texture)
                    (output / name).write_bytes(encoded)
                    record['images'].append(dict(image=image, decoded=True,
                        width=texture.width, height=texture.height, preview=name,
                        sha256=hashlib.sha256(encoded).hexdigest()))
                    figures.append(f'<figure><img loading="lazy" src="{name}">'
                        f'<figcaption>{html.escape(name)} · '
                        f'{texture.width}×{texture.height}</figcaption></figure>')
                finally:
                    decoder.nflstreet_texture_close(ctypes.byref(texture))
    report = dict(confidence='candidate UI decoding and screen roles; source bytes hashed',
                  limits=['Palette zero and base images only; original menu layouts, fonts, '
                          'screen scheduling and input transitions not reconstructed'],
                  decoder_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  records=records)
    (output / 'inventory.json').write_text(json.dumps(report, indent=2) + '\n')
    (output / 'gallery.html').write_text('<!doctype html><meta charset="utf-8">'
        '<title>Original NFL Street UI assets</title><style>'
        'body{background:#222;color:white;font:16px sans-serif}main{display:flex;'
        'flex-wrap:wrap}figure{margin:12px}img{max-width:512px;max-height:320px;'
        'object-fit:contain;background:#555}figcaption{max-width:512px}</style>'
        '<h1>Original UI asset previews</h1><p>Asset inventory; original screen '
        'layout and behavior are not reconstructed.</p><main>' + ''.join(figures) + '</main>')
    print(json.dumps(dict(entries=len(records), previews=len(figures), output=str(output))))


if __name__ == '__main__':
    main()
