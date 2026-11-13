#!/usr/bin/env python3
# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0
"""Split a tlkdbg UART stream into VCD words and LOG records.

Input is either a uarttool NDJSON trace (default) or a raw binary capture
(--raw). Frame layout:

    [A5 5A][ch][len:2 LE][payload][crc16 LE]

CRC is CRC-16/CCITT-FALSE over [ch][len_lo][len_hi][payload].
"""

import argparse
import json
import struct
import sys

CH_LOG = 0x01
CH_VCD = 0x02


def read_ndjson(path):
    out = bytearray()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue
            if rec.get("dir") != "rx":
                continue
            raw = rec.get("raw")
            if raw:
                out += bytes.fromhex(raw)
    return bytes(out)


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def parse_log(payload):
    if not payload or len(payload) < 3:
        return ("short", payload.hex())
    rtype = payload[0] & 7
    tick = payload[0] >> 3 | (payload[1] << 5) | (payload[2] << 13)
    if rtype == 5:
        ll = payload[3]
        label = payload[4:4 + ll].decode("latin1", "replace")
        n = payload[4 + ll]
        data = payload[5 + ll:5 + ll + n]
        return ("dynamic", tick, label, data)
    if rtype == 4:
        h = payload[3] | (payload[4] << 8)
        n = payload[5]
        return ("hash", tick, h, payload[6:6 + n])
    if rtype == 6:
        seed = payload[3] | (payload[4] << 8)
        th = payload[5] | (payload[6] << 8) | (payload[7] << 16)
        return ("table", tick, seed, th)
    return (f"type{rtype}", tick, payload[3:].hex())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("--raw", action="store_true", help="input is raw binary")
    ap.add_argument("--vcd-out", help="write VCD payload words here")
    ap.add_argument("--log-out", help="write decoded LOG text here")
    ap.add_argument("--show", type=int, default=10, help="print first N LOG records")
    args = ap.parse_args()

    data = open(args.input, "rb").read() if args.raw else read_ndjson(args.input)
    print(f"bytes={len(data)}")

    counts = {CH_LOG: 0, CH_VCD: 0}
    crc_err = 0
    bad_len = 0
    resync = 0
    vcd = bytearray()
    log_records = []

    i = 0
    while i + 5 <= len(data):
        if data[i] != 0xA5 or data[i + 1] != 0x5A:
            i += 1
            resync += 1
            continue
        ch = data[i + 2]
        ln = data[i + 3] | (data[i + 4] << 8)
        if ln > 256:
            bad_len += 1
            i += 2
            continue
        end = i + 5 + ln + 2
        if end > len(data):
            break
        payload = data[i + 5:i + 5 + ln]
        crc_rx = data[i + 5 + ln] | (data[i + 5 + ln + 1] << 8)
        crc_calc = crc16(data[i + 2:i + 5 + ln])
        if crc_rx != crc_calc:
            crc_err += 1
            i += 2
            continue
        counts[ch] = counts.get(ch, 0) + 1
        if ch == CH_VCD:
            vcd += payload
        else:
            log_records.append(payload)
        i = end

    print(f"frames: log={counts.get(CH_LOG, 0)} vcd={counts.get(CH_VCD, 0)}")
    print(f"crc_err={crc_err} bad_len={bad_len} resync_bytes={resync} "
          f"trailing={len(data) - i}")
    print(f"vcd_words={len(vcd) // 4}")

    decoded = [parse_log(p) for p in log_records]
    labels = {}
    for rec in decoded:
        if rec[0] == "dynamic":
            labels[rec[2]] = labels.get(rec[2], 0) + 1
    print(f"log_labels={labels}")

    for rec in decoded[:args.show]:
        if rec[0] == "dynamic":
            print(f"  LOG '{rec[2]}' data={rec[3].hex()}")
        elif rec[0] == "hash":
            print(f"  LOG <hash:0x{rec[2]:04x}> data={rec[3].hex()}")
        elif rec[0] == "table":
            print(f"  LOG table seed={rec[2]} hash=0x{rec[3]:06x}")
        else:
            print(f"  LOG {rec}")

    if args.vcd_out:
        with open(args.vcd_out, "wb") as f:
            f.write(vcd)
        print(f"wrote {args.vcd_out}")

    if args.log_out:
        with open(args.log_out, "w", encoding="utf-8") as f:
            for rec in decoded:
                if rec[0] == "dynamic":
                    if len(rec[3]) == 4:
                        val = struct.unpack("<I", rec[3])[0]
                        f.write(f"{rec[2]}={val}\n")
                    else:
                        f.write(f"{rec[2]}={rec[3].hex()}\n")
                elif rec[0] == "hash":
                    f.write(f"<hash:0x{rec[2]:04x}>={rec[3].hex()}\n")
                elif rec[0] == "table":
                    f.write(f"TABLE seed={rec[2]} hash=0x{rec[3]:06x}\n")
        print(f"wrote {args.log_out}")

    return 0 if crc_err == 0 and counts.get(CH_VCD, 0) > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
