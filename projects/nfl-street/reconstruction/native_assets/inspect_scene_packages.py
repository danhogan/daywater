"""Offline QKL resource identity checks against independently decoded DAT entries.

The descriptor layout is candidate until checked against the original loader.
Only kind-one payloads are compared; other kinds are retained as unknown.
Does not interpret prop positions, transforms, or original scene scheduling.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_scene import info

def read_package(path, unpacked):
    data = path.read_bytes()
    assert data[:4] == b'QL01'
    u32 = lambda at: struct.unpack_from('>I', data, at)[0]
    assert u32(4) == 12
    base = u32(8)
    assert 12 <= base <= len(data)-8 and data[base:base+4] == b'DATA'
    files, records = None, None
    at = 12
    while at < base:
        tag, size = data[at:at+4], u32(at+4)
        assert size >= 12 and at+size <= base
        count = u32(at+8)
        if tag == b'FILS':
            assert files is None and size == 12+count*48
            files = []
            for i in range(count):
                raw = data[at+12+i*48:at+12+(i+1)*48]
                assert b'\0' in raw
                name = raw.split(b'\0')[0].decode('ascii')
                assert Path(name).name == name and '/' not in name and '\\' not in name
                files.append(name)
        elif tag == b'DTLS':
            assert records is None and size == 12+count*12
            records = [struct.unpack_from('>HHII', data, at+12+i*12) for i in range(count)]
        else:
            raise ValueError(f'Unknown QKL header chunk {tag!r}')
        at += size
    assert at == base and files is not None and records is not None
    matches, differences, kinds, models, candidates = [], [], {}, [], []
    for kind, file_index, entry, offset in records:
        kinds[str(kind)] = kinds.get(str(kind), 0)+1
        assert file_index < len(files) and base+offset <= len(data)
        archive = files[file_index]
        if kind != 1:
            continue
        source = unpacked/Path(archive).stem/f'{entry:05d}.bin'
        if not source.exists():
            raise ValueError(f'Missing independent original entry: {source}')
        expected = source.read_bytes()
        actual = data[base+offset:base+offset+len(expected)]
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b), min(len(actual), len(expected)))
            differences.append(dict(archive=archive, entry=entry, first_difference=first,
                                    note='Package payload differs; native processing or descriptor meaning unresolved'))
            continue
        matches.append(dict(archive=archive, entry=entry, package_offset=base+offset,
                            bytes=len(expected), sha256=hashlib.sha256(expected).hexdigest()))
        if expected[:4] == b'TMdl':
            models.append(dict(archive=archive, entry=entry, name=info(expected)))
        if archive == 'envrnmt.dat' and len(expected) >= 4 and expected[:4] == b'\0\0\0i':
            candidates.append(dict(entry=entry, bytes=len(expected), version=105,
                                   note='Contains collision-like arrays and named prop/actor records; transforms not interpreted'))
    return dict(package=path.name, sha256=hashlib.sha256(data).hexdigest(),
                descriptor_kinds=kinds, exact_entry_matches=matches, differing_entries=differences, models=models,
                environment_records=candidates)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--packages', type=Path, required=True)
    p.add_argument('--unpacked', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    manifest = json.loads((args.packages/'scene-package-manifest.json').read_text())
    assert manifest['iso_sha1'] == '813c4511ba3135059a13de424555647e8252254b'
    for record in manifest['files']:
        assert hashlib.sha256((args.packages/record['path']).read_bytes()).hexdigest() == record['sha256']
    results = [read_package(path, args.unpacked) for path in sorted(args.packages.glob('game*.qkl'))]
    report = dict(confidence='candidate', original_loader_exercised=False, packages=results)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(packages=len(results), matched_entries=sum(len(r['exact_entry_matches']) for r in results),
                         differing_entries=sum(len(r['differing_entries']) for r in results),
                         original_environment_records=sum(len(r['environment_records']) for r in results))))

if __name__ == '__main__':
    main()
