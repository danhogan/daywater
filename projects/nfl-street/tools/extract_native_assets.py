"""Extract DAT assets or QKL scene packages from the hash-checked GNNE69 FST into private W: storage."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path, PurePosixPath

ISO_SHA1 = '813c4511ba3135059a13de424555647e8252254b'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--iso', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--qkl-only', action='store_true', help='Extract quick-load packages instead of DAT archives into a separate private directory')
    args = p.parse_args()
    output = args.output.resolve()
    output.relative_to(Path('/mnt/w/Daywater').resolve())
    digest = hashlib.sha1()
    records = []
    with args.iso.open('rb') as source:
        for block in iter(lambda: source.read(8*1024*1024), b''):
            digest.update(block)
        if digest.hexdigest() != ISO_SHA1:
            raise ValueError('Wrong image or revision')
        image_size = source.tell()
        source.seek(0x424)
        offset, size = struct.unpack('>II', source.read(8))
        if size > 16*1024*1024 or offset+size > image_size:
            raise ValueError('Invalid FST range')
        source.seek(offset)
        fst = source.read(size)
        count = struct.unpack_from('>I', fst, 8)[0]
        if not 1 <= count <= len(fst)//12:
            raise ValueError('Invalid FST count')
        strings = fst[count*12:]
        stack = []
        seen = set()
        # Validate the complete directory before any extraction.
        for i in range(1, count):
            while stack and i >= stack[-1][1]:
                stack.pop()
            word, start, length = struct.unpack_from('>III', fst, i*12)
            name_offset = word & 0xffffff
            end = strings.index(0, name_offset)
            name = strings[name_offset:end].decode('ascii')
            if not name or name in ('.', '..') or '/' in name or '\\' in name or ':' in name:
                raise ValueError('Unsafe FST name')
            path = PurePosixPath(*[s[0] for s in stack], name)
            if word >> 24:
                if word >> 24 != 1 or not i < length <= (stack[-1][1] if stack else count):
                    raise ValueError('Invalid directory range')
                stack.append((name, length))
                continue
            if start+length > image_size or str(path) in seen:
                raise ValueError('Invalid or duplicate file')
            seen.add(str(path))
            if path.suffix.lower() == ('.qkl' if args.qkl_only else '.dat'):
                records.append(dict(path=str(path), offset=start, size=length))
        output.mkdir(parents=True, exist_ok=True)
        for r in records:
            dest = output/r['path']
            dest.resolve().relative_to(output)
            source.seek(r['offset'])
            data = source.read(r['size'])
            r['sha256'] = hashlib.sha256(data).hexdigest()
            dest.parent.mkdir(parents=True, exist_ok=True)
            if dest.exists():
                if hashlib.sha256(dest.read_bytes()).hexdigest() != r['sha256']:
                    raise ValueError(f'Existing file differs: {dest}')
            else:
                with dest.open('xb') as target:
                    target.write(data)
        manifest = dict(iso_sha1=ISO_SHA1, scope='QKL packages' if args.qkl_only else 'DAT assets; not audio/video', files=records)
        (output/('scene-package-manifest.json' if args.qkl_only else 'asset-manifest.json')).write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(files=len(records), bytes=sum(r['size'] for r in records), output=str(output))))

if __name__ == '__main__':
    main()
