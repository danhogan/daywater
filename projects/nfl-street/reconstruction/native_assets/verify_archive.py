"""Independent offline check of native Windows unpacker output against original DAT bytes.

All plain entries are checked. Codec 5 is independently decoded for loaddata
and plamodel, with tables/counts read from the hash-checked original DOL.
The native decoder checks embedded checksums for every compressed entry.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'

class Bits:
    def __init__(self, data):
        self.data, self.bit = data, 0

    def get(self, count):
        value = 0
        for _ in range(count):
            if self.bit // 8 >= len(self.data):
                raise ValueError('Truncated stream')
            value = value*2 + ((self.data[self.bit//8] >> (7-self.bit % 8)) & 1)
            self.bit += 1
        return value

    def tree(self, count):
        lengths = [self.get(4) for _ in range(count)]
        codes = [0]*16
        counts = [lengths.count(i) for i in range(16)]
        counts[0] = 0
        for i in range(1, 16):
            codes[i] = (codes[i-1]+counts[i-1])*2
            if codes[i]+counts[i] > 1 << i:
                raise ValueError('Oversubscribed tree')
        lookup = {}
        for symbol, length in enumerate(lengths):
            if length:
                lookup[length, codes[length]] = symbol
                codes[length] += 1
        return lookup

    def symbol(self, lookup):
        value = 0
        for length in range(1, 16):
            value = value*2+self.get(1)
            if (length, value) in lookup:
                return lookup[length, value]
        raise ValueError('Invalid Huffman code')

def dol_reader(path):
    data = path.read_bytes()
    if hashlib.sha1(data).hexdigest() != DOL_SHA1:
        raise ValueError('Wrong original module')
    def read(address, size):
        for i in range(18):
            off, base, length = [struct.unpack_from('>I', data, h+4*i)[0] for h in (0, 0x48, 0x90)]
            if base <= address and address+size <= base+length:
                return data[off+address-base:off+address-base+size]
        raise ValueError('Address outside original module')
    return read

def decode(data, size, read):
    literal_count = struct.unpack('>H', read(0x8026A1D0, 2))[0]
    distance_count = struct.unpack('>H', read(0x8026A1DC, 2))[0]
    length_base = struct.unpack('>28H', read(0x8026A1FC, 56))
    length_bits = read(0x8026A1E0, 28)
    distance_base = struct.unpack('>30H', read(0x8026A256, 60))
    distance_bits = read(0x8026A238, 30)
    bits, out = Bits(data), bytearray()
    while not bits.get(1):
        literals, distances = bits.tree(literal_count), bits.tree(distance_count)
        while True:
            s = bits.symbol(literals)
            if s == 256:
                break
            if s < 256:
                out.append(s)
            else:
                i = s-257
                length = length_base[i]+bits.get(length_bits[i])
                d = bits.symbol(distances)
                distance = distance_base[d]+bits.get(distance_bits[d])
                if not 0 < distance <= len(out):
                    raise ValueError('Invalid back reference')
                for _ in range(length):
                    out.append(out[-distance])
            if len(out) > size:
                raise ValueError('Output exceeds directory size')
    if bits.get(32) != zlib.adler32(out) or len(out) != size:
        raise ValueError('Checksum or decoded size differs')
    return out

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dol', 'assets', 'unpacked', 'report'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    args.report.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    read = dol_reader(args.dol)
    records = []
    for path in sorted(args.assets.glob('*.dat')):
        data = path.read_bytes()
        if data[:4] != b'TERF':
            raise ValueError('Not TERF')
        p, chunks = struct.unpack_from('>I', data, 4)[0], {}
        while p < len(data):
            tag, length = data[p:p+4], struct.unpack_from('>I', data, p+4)[0]
            if length < 8 or length > len(data)-p:
                raise ValueError('Invalid chunk')
            chunks[tag] = p
            p += length
        count = struct.unpack_from('>H', data, 14)[0]
        plain, reference = 0, 0
        for i in range(count):
            off, size = struct.unpack_from('>II', data, chunks[b'DIR1']+8+i*8)
            codec, decoded_size = struct.unpack_from('>II', data, chunks[b'COMP']+8+i*8) if b'COMP' in chunks else (0, size)
            raw = data[chunks[b'DATA']+off:chunks[b'DATA']+off+size]
            native = args.unpacked/path.stem/f'{i:05d}.bin'
            actual = native.read_bytes()
            if len(actual) != decoded_size:
                raise ValueError(f'Native size differs: {native}')
            if not codec:
                if actual != raw:
                    raise ValueError(f'Plain entry differs: {native}')
                plain += 1
            elif codec == 5 and path.stem in ('loaddata', 'plamodel'):
                if actual != decode(raw, decoded_size, read):
                    raise ValueError(f'Independent decompression differs: {native}')
                reference += 1
        records.append(dict(archive=path.name, entries=count, plain_compared=plain,
                            independently_decoded=reference, all_sizes_checked=True))
    result = dict(dol_sha1=DOL_SHA1, confidence='probable', live_original_decoder_comparison=False,
                  archives=records, plain_compared=sum(r['plain_compared'] for r in records),
                  independently_decoded=sum(r['independently_decoded'] for r in records))
    args.report.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'archives'}))

if __name__ == '__main__':
    main()
