"""Small binary reader for the LLVM core modules used by the WASI adapter."""
import json
from pathlib import Path


def leb(value):
    out = bytearray()
    while value >= 128:
        out.append((value & 127) | 128)
        value >>= 7
    out.append(value)
    return bytes(out)


def string(value):
    data = value.encode()
    return leb(len(data)) + data


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def byte(self):
        value = self.data[self.pos]
        self.pos += 1
        return value

    def uint(self):
        value = shift = 0
        while True:
            byte = self.byte()
            value |= (byte & 127) << shift
            if byte < 128:
                return value
            shift += 7
            if shift > 63:
                raise ValueError('invalid integer')

    def take(self, size):
        value = self.data[self.pos:self.pos + size]
        if len(value) != size:
            raise ValueError('truncated module')
        self.pos += size
        return value

    def string(self):
        return self.take(self.uint()).decode()

    def vector(self, read):
        return [read() for _ in range(self.uint())]

    def limits(self):
        flags = self.uint()
        minimum = self.uint()
        maximum = self.uint() if flags & 1 else None
        if flags & ~7:
            raise ValueError('unsupported memory/table limits')
        return flags, minimum, maximum


VALUES = {0x7f: 'i32', 0x7e: 'i64', 0x7d: 'f32', 0x7c: 'f64', 0x70: 'funcref'}


class Module:
    def __init__(self, data):
        self.data = data
        r = Reader(data)
        assert r.take(8) == b'\0asm\1\0\0\0'
        self.sections, self.types, self.imports, self.exports = [], [], [], {}
        self.functions, self.memories, self.metadata, self.tables = [], [], [], []
        while r.pos < len(data):
            kind, payload = r.byte(), r.take(r.uint())
            self.sections.append((kind, payload))
            s = Reader(payload)
            if kind == 0:
                if s.string().startswith('component-type'):
                    self.metadata.append(payload)
            elif kind == 1:
                for _ in range(s.uint()):
                    assert s.byte() == 0x60, 'expected LLVM function type'
                    self.types.append((s.vector(lambda: VALUES[s.byte()]), s.vector(lambda: VALUES[s.byte()])))
            elif kind == 2:
                for _ in range(s.uint()):
                    module, name, ty = s.string(), s.string(), s.byte()
                    if ty == 0:
                        desc = s.uint()
                        self.functions.append(desc)
                    elif ty == 1:
                        desc = (s.byte(), s.limits())
                        self.tables.append(desc)
                    elif ty == 2:
                        desc = s.limits()
                        self.memories.append(desc)
                    elif ty == 3:
                        desc = (s.byte(), s.byte())
                    elif ty == 4:
                        desc = (s.byte(), s.uint())
                    else:
                        raise ValueError(f'unsupported import {ty}')
                    self.imports.append((module, name, ty, desc))
            elif kind == 3:
                self.functions.extend(s.vector(s.uint))
            elif kind == 4:
                self.tables.extend(s.vector(lambda: (s.byte(), s.limits())))
            elif kind == 5:
                self.memories.extend(s.vector(s.limits))
            elif kind == 7:
                for _ in range(s.uint()):
                    name = s.string()
                    self.exports[name] = (s.byte(), s.uint())

    def signature(self, name):
        kind, index = self.exports[name]
        assert kind == 0
        return self.types[self.functions[index]]

    def abi(self):
        return {
            'pointerSize': 8 if self.memories[0][0] & 4 else 4,
            'memoryMinimum': self.memories[0][1],
            'memoryMaximum': self.memories[0][2],
            'table64': bool(self.tables and self.tables[0][1][0] & 4),
            'imports': {f'{module}#{name}': self.types[index] for module, name, kind, index in self.imports if kind == 0},
            'exports': {name: self.signature(name) for name, (kind, _) in self.exports.items() if kind == 0},
        }

    def import_memory(self):
        if any(kind == 2 for _, _, kind, _ in self.imports):
            return self.data
        assert len(self.memories) == 1
        limits = next(payload for kind, payload in self.sections if kind == 5)
        reader = Reader(limits)
        assert reader.uint() == 1
        entry = string('env') + string('memory') + b'\x02' + limits[reader.pos:]
        sections = []
        inserted = False
        for kind, payload in self.sections:
            if kind == 5:
                continue
            if kind == 2:
                reader = Reader(payload)
                count = reader.uint()
                payload = leb(count + 1) + payload[reader.pos:] + entry
                inserted = True
            elif kind > 2 and not inserted:
                sections.append((2, leb(1) + entry))
                inserted = True
            sections.append((kind, payload))
        return b'\0asm\1\0\0\0' + b''.join(bytes([k]) + leb(len(p)) + p for k, p in sections)


def add_metadata(data, metadata):
    return data + b''.join(b'\0' + leb(len(p)) + p for p in metadata)


def custom(data, name, payload):
    return add_metadata(data, [string(name) + payload])


def quote(value):
    return json.dumps(value)


def binary(data):
    return '"' + ''.join(f'\\{b:02x}' for b in data) + '"'


if __name__ == '__main__':
    import sys
    source, destination = map(Path, sys.argv[1:])
    data = source.read_bytes()
    destination.write_bytes(custom(data, 'csound.abi', json.dumps(Module(data).abi()).encode()))
