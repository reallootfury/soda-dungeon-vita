#!/usr/bin/env python3
"""Max the active Soda Dungeon slot inside this port's preference container."""

import re
import struct
import sys
import urllib.parse


class Node:
    def __init__(self, kind, value, start, end, children=None):
        self.kind = kind
        self.value = value
        self.start = start
        self.end = end
        self.children = children or []


class HaxeParser:
    def __init__(self, text):
        self.text = text
        self.pos = 0
        self.strings = []
        self.string_nodes = []
        self.references = []

    def number(self):
        start = self.pos
        if self.text[self.pos:self.pos + 1] == "-":
            self.pos += 1
        while self.pos < len(self.text) and self.text[self.pos].isdigit():
            self.pos += 1
        return int(self.text[start:self.pos])

    def value(self):
        start = self.pos
        token = self.text[self.pos]
        self.pos += 1
        if token in "ntfz":
            return Node("scalar", {"n": None, "t": True, "f": False,
                                   "z": 0}[token], start, self.pos)
        if token == "i":
            return Node("int", self.number(), start, self.pos)
        if token == "d":
            number_start = self.pos
            while (self.pos < len(self.text) and
                   self.text[self.pos] in "-+.0123456789eE"):
                self.pos += 1
            return Node("float", float(self.text[number_start:self.pos]),
                        start, self.pos)
        if token == "y":
            length = self.number()
            if self.text[self.pos] != ":":
                raise ValueError("invalid Haxe string")
            self.pos += 1
            value = self.text[self.pos:self.pos + length]
            self.pos += length
            node = Node("string", value, start, self.pos)
            self.strings.append(value)
            self.string_nodes.append(node)
            return node
        if token == "R":
            index = self.number()
            node = Node("string_ref", index, start, self.pos)
            self.references.append(node)
            return node
        if token == "r":
            return Node("object_ref", self.number(), start, self.pos)
        if token == "v":
            return Node("date", self.number(), start, self.pos)
        if token == "u":
            return Node("null_run", self.number(), start, self.pos)
        if token == "a":
            children = []
            while self.text[self.pos] != "h":
                children.append(self.value())
            self.pos += 1
            return Node("array", None, start, self.pos, children)
        if token in "ob":
            children = []
            end_token = "g" if token == "o" else "h"
            while self.text[self.pos] != end_token:
                children.extend((self.value(), self.value()))
            self.pos += 1
            return Node("object" if token == "o" else "string_map",
                        None, start, self.pos, children)
        if token == "q":
            children = []
            while self.text[self.pos] != "h":
                if self.text[self.pos] != ":":
                    raise ValueError("invalid Haxe IntMap")
                self.pos += 1
                self.number()
                children.append(self.value())
            self.pos += 1
            return Node("int_map", None, start, self.pos, children)
        if token in "Cc":
            name = self.value()
            children = []
            while self.text[self.pos] != "g":
                if token == "C":
                    children.append(self.value())
                else:
                    children.extend((self.value(), self.value()))
            self.pos += 1
            return Node("custom" if token == "C" else "class", name,
                        start, self.pos, children)
        raise ValueError("unsupported Haxe token %r at %d" % (token, start))

    def all_values(self):
        values = []
        while self.pos < len(self.text):
            values.append(self.value())
        return values


def serialized_int(value):
    return "z" if value == 0 else "i%d" % value


def class_name(node, parser):
    if node.kind != "custom":
        return None
    if node.value.kind == "string":
        return node.value.value
    if node.value.kind == "string_ref":
        return parser.strings[node.value.value]
    return None


def max_inner_save(inner):
    parser = HaxeParser(inner)
    roots = parser.all_values()
    save = next((node for node in roots
                 if class_name(node, parser) == "SaveFile"), None)
    if not save or len(save.children) != 52:
        raise ValueError("unsupported SaveFile layout")
    fields = save.children
    replacements = []

    def replace(node, text):
        replacements.append((node.start, node.end, text))

    # These are the only two fields independently verified from both the
    # serializer and live game methods. Later fields moved between save format
    # revisions, so editing them positionally can turn a required object null.
    for index, value in {
        0: 999_999_999,     # gold
        1: 999_999_999,     # essence
    }.items():
        replace(fields[index], serialized_int(value))

    for start, end, text in sorted(replacements, reverse=True):
        inner = inner[:start] + text + inner[end:]
    return inner


def max_preference_value(value):
    marker = "oy10:saveStringy"
    if not value.startswith(marker):
        return value
    length_end = value.index(":", len(marker))
    length = int(value[len(marker):length_end])
    encoded_start = length_end + 1
    encoded = value[encoded_start:encoded_start + length]
    inner = urllib.parse.unquote(encoded)
    inner = max_inner_save(inner)
    encoded = urllib.parse.quote(inner, safe="")
    return marker + str(len(encoded)) + ":" + encoded + value[encoded_start + length:]


def max_file(path):
    data = open(path, "rb").read()
    if data[:8] != b"SDPREF01":
        raise ValueError("not a Soda Dungeon Vita preference file")
    count = struct.unpack_from("<I", data, 8)[0]
    offset = 12
    entries = []
    for _ in range(count):
        key_length, value_length = struct.unpack_from("<II", data, offset)
        offset += 8
        key = data[offset:offset + key_length]
        offset += key_length
        value = data[offset:offset + value_length].decode("utf-8")
        offset += value_length
        entries.append((key, max_preference_value(value).encode("utf-8")))
    output = bytearray(b"SDPREF01" + struct.pack("<I", len(entries)))
    for key, value in entries:
        output += struct.pack("<II", len(key), len(value)) + key + value
    with open(path, "wb") as stream:
        stream.write(output)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: max-save.py preferences.bin [preferences.bak]")
    for filename in sys.argv[1:]:
        max_file(filename)
        print("maxed", filename)
