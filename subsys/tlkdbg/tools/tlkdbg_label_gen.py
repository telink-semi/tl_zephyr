#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""tlkdbg_label_gen.py — generate build-time LOG label hashes.

Scans the source tree for

    LOG_STR_DATA(tag, "label text", data, n)

and produces:

    tlkdbg_label_hashes.h   #define TLKDBG_LABEL_HASH_<tag> <hash>
    <bin>.labels.json       {"0x1234": "label text", ...}

The 16-bit hash is seeded; the script searches for a seed that makes every
label unique. The device never stores the string or a label table: it only
uses the generated constant.

This is the Scheme-B prototype. Labels may be C string literals (including
adjacent literals and the common escapes). Dynamic labels should use
LOG_STR_DYNAMIC() and are deliberately not part of this table.
"""
import argparse
import json
import os
import re
import sys

# Directory names that never contain application log calls.
SKIP_DIRS = {
    '3rd-party', 'cmake_builds', 'cmake_configs', 'tools', 'doc',
    '.git', 'node_modules', '__pycache__',
}

CALL_RE = re.compile(r'\bLOG_STR_DATA\s*\(\s*([A-Za-z_]\w*)\s*,\s*')
OLD_CALL_RE = re.compile(r'\blog_str_data\s*\(\s*"')

ESCAPES = {
    'n': '\n', 'r': '\r', 't': '\t', '0': '\0',
    '\\': '\\', '"': '"', "'": "'", 'a': '\a', 'b': '\b',
    'f': '\f', 'v': '\v',
}


def parse_c_string(text, pos):
    """Parse one or more adjacent C string literals starting at text[pos].

    Returns (decoded_string, new_pos).  Raises ValueError on malformed input.
    """
    out = []
    seen = False
    while pos < len(text):
        while pos < len(text) and text[pos] in ' \t\r\n':
            pos += 1
        if pos >= len(text) or text[pos] != '"':
            break
        seen = True
        pos += 1
        while pos < len(text):
            ch = text[pos]
            if ch == '"':
                pos += 1
                break
            if ch != '\\':
                out.append(ch)
                pos += 1
                continue
            pos += 1
            if pos >= len(text):
                raise ValueError('trailing backslash')
            esc = text[pos]
            pos += 1
            if esc in ESCAPES:
                out.append(ESCAPES[esc])
            elif esc == 'x':
                j = pos
                while j < len(text) and text[j] in '0123456789abcdefABCDEF':
                    j += 1
                if j == pos:
                    raise ValueError('bad \\x escape')
                out.append(chr(int(text[pos:j], 16) & 0xff))
                pos = j
            elif esc in '01234567':
                j = pos - 1
                while j < len(text) and j < pos + 2 and text[j] in '01234567':
                    j += 1
                out.append(chr(int(text[pos - 1:j], 8) & 0xff))
                pos = j
            else:
                out.append(esc)
        else:
            raise ValueError('unterminated string')
    if not seen:
        raise ValueError('no string literal')
    return ''.join(out), pos


def strip_comments(text):
    """Replace C comments with spaces, keeping newlines and string contents."""
    out = []
    i = 0
    n = len(text)
    state = 'code'
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state == 'code':
            if ch == '/' and nxt == '*':
                state = 'block'
                out.append('  ')
                i += 2
                continue
            if ch == '/' and nxt == '/':
                state = 'line'
                out.append('  ')
                i += 2
                continue
            if ch == '"':
                state = 'string'
            out.append(ch)
            i += 1
            continue
        if state == 'string':
            out.append(ch)
            if ch == '\\' and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if ch == '"':
                state = 'code'
            i += 1
            continue
        if state == 'block':
            if ch == '*' and nxt == '/':
                state = 'code'
                out.append('  ')
                i += 2
                continue
            out.append('\n' if ch == '\n' else ' ')
            i += 1
            continue
        # line comment
        if ch == '\n':
            state = 'code'
            out.append('\n')
        else:
            out.append(' ')
        i += 1
    return ''.join(out)


def iter_sources(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            if name.endswith(('.c', '.h')):
                yield os.path.join(dirpath, name)


def collect(sources):
    """Return {tag: {string, file, line}} and a list of old-style call files."""
    tags = {}
    old = []
    conflicts = []
    for path in sources:
        try:
            with open(path, 'r', encoding='utf-8', errors='replace') as f:
                text = strip_comments(f.read())
        except OSError:
            continue
        for m in CALL_RE.finditer(text):
            line_start = text.rfind('\n', 0, m.start()) + 1
            if '#define' in text[line_start:m.start()]:
                continue              # the macro definition itself, not a call
            tag = m.group(1)
            try:
                label, _ = parse_c_string(text, m.end())
            except ValueError as e:
                print('%s:%d: cannot parse label: %s'
                      % (path, text.count('\n', 0, m.start()) + 1, e),
                      file=sys.stderr)
                continue
            line = text.count('\n', 0, m.start()) + 1
            prev = tags.get(tag)
            if prev is not None and prev['string'] != label:
                conflicts.append((tag, prev, {
                    'string': label, 'file': path, 'line': line}))
            else:
                tags.setdefault(tag, {'string': label, 'file': path, 'line': line})
        if OLD_CALL_RE.search(text):
            old.append(path)
    if conflicts:
        for tag, a, b in conflicts:
            print('ERROR: tag %r used with two different labels:\n'
                  '  %s:%d %r\n  %s:%d %r'
                  % (tag, a['file'], a['line'], a['string'],
                     b['file'], b['line'], b['string']), file=sys.stderr)
        raise SystemExit(2)
    return tags, old


def fnv1a16(text, seed):
    h = (0x811c9dc5 ^ (seed & 0xffff)) & 0xffffffff
    for b in text.encode('utf-8'):
        h ^= b
        h = (h * 16777619) & 0xffffffff
    return ((h ^ (h >> 16)) & 0xffff)


def fnv1a24(text):
    h = 0x811c9dc5
    for b in text.encode('utf-8'):
        h ^= b
        h = (h * 16777619) & 0xffffffff
    return h & 0xffffff


def find_seed(strings):
    """Find a 16-bit seed with no collisions over the distinct label strings."""
    uniq = sorted(set(strings))
    for seed in range(0x10000):
        seen = {}
        ok = True
        for s in uniq:
            h = fnv1a16(s, seed)
            if h in seen:
                ok = False
                break
            seen[h] = s
        if ok:
            return seed, {s: fnv1a16(s, seed) for s in uniq}
    raise SystemExit('ERROR: no collision-free 16-bit seed; use a 32-bit hash '
                     'or split the label set')


def write_header(path, seed, table_hash, tags, hashes):
    lines = [
        '/* Auto-generated by tools/tlkdbg_label_gen.py. Do not edit. */',
        '#ifndef TLKDBG_LABEL_HASHES_H_',
        '#define TLKDBG_LABEL_HASHES_H_',
        '',
        '#define TLKDBG_LABEL_HASH_SEED   0x%04xu' % seed,
        '#define TLKDBG_LABEL_TABLE_HASH  0x%06xu' % table_hash,
        '',
    ]
    for tag in sorted(tags):
        h = hashes[tags[tag]['string']]
        lines.append('#define TLKDBG_LABEL_HASH_%-24s 0x%04xu' % (tag, h))
    lines += ['', '#endif', '']
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))


def write_table(path, seed, table_hash, tags, hashes):
    labels = {}
    for tag in sorted(tags):
        s = tags[tag]['string']
        labels['0x%04x' % hashes[s]] = s
    data = {
        'schema': 'tlkdbg.labels/1',
        'seed': seed,
        'table_hash': '0x%06x' % table_hash,
        'labels': labels,
    }
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.write('\n')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', default='.', help='source root (default: .)')
    ap.add_argument('--roots', default='common,demo,project',
                    help='comma-separated subdirectories to scan')
    ap.add_argument('--out-header', default='build/generated/tlkdbg_label_hashes.h')
    ap.add_argument('--out-table', required=True,
                    help='path of <bin>.labels.json')
    ap.add_argument('--verbose', action='store_true',
                    help='print every tag/hash (default: first/last few)')
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    sources = []
    for sub in args.roots.split(','):
        sub = sub.strip()
        if not sub:
            continue
        p = os.path.join(root, sub)
        if os.path.isdir(p):
            sources.extend(iter_sources(p))
    tags, old = collect(sources)
    if not tags:
        raise SystemExit('ERROR: no LOG_STR_DATA(tag, "text", ...) call found '
                         'under %s' % root)

    strings = sorted({v['string'] for v in tags.values()})
    seed, hashes = find_seed(strings)

    # Table digest: over sorted (hash, label) pairs, so it changes whenever the
    # set of labels or a label's text changes.
    digest_lines = sorted('0x%04x\t%s' % (hashes[s], s) for s in strings)
    table_hash = fnv1a24('\n'.join(digest_lines))

    out_header = os.path.abspath(args.out_header)
    out_table = os.path.abspath(args.out_table)
    os.makedirs(os.path.dirname(out_header), exist_ok=True)
    os.makedirs(os.path.dirname(out_table), exist_ok=True)
    write_header(out_header, seed, table_hash, tags, hashes)
    write_table(out_table, seed, table_hash, tags, hashes)

    print('labels: %d unique, %d tags' % (len(strings), len(tags)))
    print('seed: 0x%04x, table_hash: 0x%06x' % (seed, table_hash))
    print('header: %s' % out_header)
    print('table : %s' % out_table)
    ordered = sorted(tags)
    if args.verbose or len(ordered) <= 16:
        shown = ordered
    else:
        shown = ordered[:8] + [None] + ordered[-8:]
    for tag in shown:
        if tag is None:
            print('  ... %d more ...' % (len(ordered) - 16))
            continue
        s = tags[tag]['string']
        print('  0x%04x  %-20s %r' % (hashes[s], tag, s))
    if old:
        print('WARNING: old log_str_data() calls still present in:', file=sys.stderr)
        for p in old:
            print('  %s' % p, file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
