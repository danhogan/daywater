"""Export the checked card-question strings from GNNE69-r0 to private W: storage."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'
# Original 80152EC8 code3F and 8018E340/8018E2C4 references. No strings in repo.
ADDRESSES = (0x8025E694, 0x8025EF58, 0x8025EFB4, 0x8025EFE0,
             0x8025EFF8, 0x80262074, 0x80262058, 0x80262064)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dol', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data = args.dol.read_bytes()
    if hashlib.sha1(data).hexdigest() != DOL_SHA1:
        raise ValueError('Unsupported original DOL hash')
    output = args.output.resolve()
    output.relative_to(Path('/mnt/w/Daywater').resolve())
    sections = [(struct.unpack_from('>I', data, i * 4)[0],
                 struct.unpack_from('>I', data, 0x48 + i * 4)[0],
                 struct.unpack_from('>I', data, 0x90 + i * 4)[0])
                for i in range(18)]
    records, encoded = [], bytearray()
    for index, address in enumerate(ADDRESSES):
        spans = [(file + address - start, start + size - address)
                 for file, start, size in sections if start <= address < start + size]
        if len(spans) != 1:
            raise ValueError(f'Unmapped template {index}')
        offset, remaining = spans[0]
        value = data[offset:offset + min(remaining, 256)]
        if b'\0' not in value:
            raise ValueError(f'Unterminated template {index}')
        value = value.split(b'\0', 1)[0]
        encoded.extend(value + bytes(256 - len(value)))
        records.append(dict(index=index, source_address=f'{address:08X}',
                            bytes=len(value), sha256=hashlib.sha256(value).hexdigest()))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(encoded)
    output.with_suffix('.json').write_text(json.dumps(dict(
        game='NFL Street GNNE69-r0', dol_sha1=DOL_SHA1,
        scope='code3F question, choices and device/slot names; no card I/O',
        sha256=hashlib.sha256(encoded).hexdigest(), templates=records), indent=2) + '\n')
    print(f'Exported {len(records)} templates to {output}')


if __name__ == '__main__':
    main()
