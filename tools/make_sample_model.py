#!/usr/bin/env python3
"""Regenerate the original, dependency-free sample GLB. Uses only Python's stdlib."""
from __future__ import annotations

import json
import struct
from pathlib import Path


def write_cube(destination: Path) -> None:
    # Outward-facing, counter-clockwise vertices for six faces.
    faces = [
        ([(.5, -.5, -.5), (.5, .5, -.5), (.5, .5, .5), (.5, -.5, .5)], (1, 0, 0), (.95, .42, .24, 1)),
        ([(-.5, -.5, .5), (-.5, .5, .5), (-.5, .5, -.5), (-.5, -.5, -.5)], (-1, 0, 0), (.68, .25, .16, 1)),
        ([(-.5, .5, -.5), (-.5, .5, .5), (.5, .5, .5), (.5, .5, -.5)], (0, 1, 0), (1, .74, .36, 1)),
        ([(-.5, -.5, .5), (-.5, -.5, -.5), (.5, -.5, -.5), (.5, -.5, .5)], (0, -1, 0), (.50, .21, .16, 1)),
        ([(-.5, -.5, .5), (.5, -.5, .5), (.5, .5, .5), (-.5, .5, .5)], (0, 0, 1), (.90, .34, .20, 1)),
        ([(.5, -.5, -.5), (-.5, -.5, -.5), (-.5, .5, -.5), (.5, .5, -.5)], (0, 0, -1), (.75, .29, .22, 1)),
    ]
    positions, normals, colors, indices = [], [], [], []
    for face_index, (vertices, normal, color) in enumerate(faces):
        positions.extend(component for vertex in vertices for component in vertex)
        normals.extend(component for _ in vertices for component in normal)
        colors.extend(component for _ in vertices for component in color)
        base = face_index * 4
        indices.extend(base + index for index in (0, 1, 2, 0, 2, 3))

    data = bytearray()
    views = []
    for values, code, target in ((positions, 'f', 34962), (normals, 'f', 34962),
                                  (colors, 'f', 34962), (indices, 'H', 34963)):
        chunk = struct.pack('<' + code * len(values), *values)
        views.append({'buffer': 0, 'byteOffset': len(data), 'byteLength': len(chunk), 'target': target})
        data.extend(chunk)
        data.extend(b'\0' * (-len(data) % 4))
    document = {
        'asset': {'version': '2.0', 'generator': 'C++ game template sample generator'},
        'scene': 0, 'scenes': [{'nodes': [0]}], 'nodes': [{'mesh': 0, 'name': 'Cube'}],
        'meshes': [{'primitives': [{'attributes': {'POSITION': 0, 'NORMAL': 1, 'COLOR_0': 2},
                                    'indices': 3, 'material': 0, 'mode': 4}]}],
        'materials': [{'name': 'Vertex colors', 'pbrMetallicRoughness': {
            'baseColorFactor': [1, 1, 1, 1], 'metallicFactor': 0, 'roughnessFactor': 1}}],
        'buffers': [{'byteLength': len(data)}], 'bufferViews': views,
        'accessors': [
            {'bufferView': 0, 'componentType': 5126, 'count': 24, 'type': 'VEC3',
             'min': [-.5, -.5, -.5], 'max': [.5, .5, .5]},
            {'bufferView': 1, 'componentType': 5126, 'count': 24, 'type': 'VEC3'},
            {'bufferView': 2, 'componentType': 5126, 'count': 24, 'type': 'VEC4'},
            {'bufferView': 3, 'componentType': 5123, 'count': 36, 'type': 'SCALAR'},
        ],
    }
    text = json.dumps(document, separators=(',', ':')).encode('utf-8')
    text += b' ' * (-len(text) % 4)
    total_size = 12 + 8 + len(text) + 8 + len(data)
    payload = (struct.pack('<4sII', b'glTF', 2, total_size) +
               struct.pack('<II', len(text), 0x4E4F534A) + text +
               struct.pack('<II', len(data), 0x004E4942) + data)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(payload)
    print(f'Wrote {destination} ({len(payload)} bytes)')


if __name__ == '__main__':
    write_cube(Path(__file__).resolve().parents[1] / 'assets' / 'models' / 'cube.glb')
