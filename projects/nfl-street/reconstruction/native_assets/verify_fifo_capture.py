"""Compare defined DFF v6 contents and decoded draws from two private captures.

Reserved record padding is deliberately excluded: it is not GPU state.
No Dolphin/Ghidra process is needed. All output must stay on W:.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct


class Capture:
    def __init__(self, path):
        self.data = path.read_bytes()
        if len(self.data) < 128 or self.u32(0) != 0x0D01F1F0 or self.u32(4) != 6:
            raise ValueError('unsupported capture header')

    def span(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.data) or size > len(self.data) - offset:
            raise ValueError('capture span')
        return self.data[offset:offset + size]

    def u32(self, offset):
        return struct.unpack('<I', self.span(offset, 4))[0]

    def u64(self, offset):
        return struct.unpack('<Q', self.span(offset, 8))[0]

    def digest(self):
        """Hash defined initial state, command bytes and ordered RAM updates."""
        h = hashlib.sha256()
        def add(value):
            h.update(struct.pack('<Q', len(value)))
            h.update(value)
        add(self.span(88, 16))  # RAM sizes and game id
        for offset, words in [(12, True), (24, True), (36, True), (48, True), (76, False)]:
            add(self.span(self.u64(offset), self.u32(offset + 8) * (4 if words else 1)))
        frames = []
        count = self.u32(68)
        if count > 10000:
            raise ValueError('frame count')
        self.span(self.u64(60), count * 64)
        for i in range(count):
            at = self.u64(60) + i * 64
            fifo = self.span(self.u64(at), self.u32(at + 8))
            add(fifo)
            n, offset = self.u32(at + 28), self.u64(at + 20)
            self.span(offset, n * 24)
            types = Counter()
            previous = 0
            for j in range(n):
                m = offset + j * 24
                position = self.u32(m)
                if position < previous or position > len(fifo):
                    raise ValueError('memory update order')
                previous = position
                add(self.span(m, 8))  # command position and RAM address
                add(self.span(m + 20, 1))
                add(self.span(self.u64(m + 8), self.u32(m + 16)))
                types[self.data[m + 20]] += 1
            frames.append(dict(fifo_bytes=len(fifo), fifo_sha256=hashlib.sha256(fifo).hexdigest(),
                               memory_updates=n, update_types=dict(types)))
        return h.hexdigest(), frames


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory', type=Path)
    a = p.parse_args()
    directory = a.directory.resolve()
    directory.relative_to(Path('/mnt/w/Daywater').resolve())
    captures = [Capture(directory / (name + '.dff')) for name in ['reference', 'repeat']]
    hashes = [c.digest() for c in captures]
    states = [json.loads((directory / (name + '-state.json')).read_text()) for name in ['reference', 'repeat']]
    report = dict(confidence='verified', scope='GNNE69 revision 0, supplied saved match, two fresh research JIT runs',
                  defined_capture_equal=hashes[0][0] == hashes[1][0],
                  defined_capture_sha256=[v[0] for v in hashes], frames=hashes[0][1],
                  decoded_frames_equal=states[0] == states[1], screenshot_exact_recorded_frame=False)
    draws = states[0]['frames'][-1]['draws']
    report['draw_count'] = len(draws)
    report['tev_stage_counts'] = dict(Counter(1 + ((d['bp'][0] >> 10) & 15) for d in draws))
    report['blend_enabled_counts'] = dict(Counter(d['bp'][0x41] & 1 for d in draws))
    report['depth_register_counts'] = dict(Counter(hex(d['bp'][0x40]) for d in draws))
    report['stage0_color_counts'] = dict(Counter(hex(d['bp'][0xc0]) for d in draws))
    report['stage0_alpha_counts'] = dict(Counter(hex(d['bp'][0xc1]) for d in draws))
    report['limitations'] = [
        'Frame 0 is the initial partial boundary with no draws; frame 1 is the full rendered frame.',
        'Screenshot was requested after recording; it is a nearby frame, not pixel comparison evidence.',
        'Decoded draws are state observations, not identified game objects or a native rendered result.',
        'No claim for other matches, venues, animation states, or complete GameCube rendering.',
    ]
    (directory / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['defined_capture_equal', 'decoded_frames_equal', 'draw_count', 'tev_stage_counts', 'blend_enabled_counts']}))
    if not report['defined_capture_equal'] or not report['decoded_frames_equal'] or not draws:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
