#!/usr/bin/env python3
"""Convert a Nexus pcapng capture into a radio-multi fuzzer seed corpus.

Nexus writes DLT_IEEE802_15_4_TAP (283) frames when OT_NEXUS_PCAP_FILE is set.
Real frames make far better seeds than hand-built ones: they already carry valid
IPHC compression, MLE/CoAP payloads and MAC framing that the fuzzer would
otherwise have to rediscover byte by byte.

Usage:  pcap_to_corpus.py <in.pcapng> [<in.pcapng> ...] <outdir>
"""

import os
import struct
import sys

EPB = 0x00000006
IDB = 0x00000001
SHB = 0x0A0D0D0A


def parse_pcapng(path):
    """Yield raw packet payloads from every Enhanced Packet Block."""
    data = open(path, "rb").read()
    off = 0
    endian = "<"
    while off + 12 <= len(data):
        (btype,) = struct.unpack_from(endian + "I", data, off)
        if btype == SHB:
            (magic,) = struct.unpack_from("<I", data, off + 8)
            endian = "<" if magic == 0x1A2B3C4D else ">"
        (blen,) = struct.unpack_from(endian + "I", data, off + 4)
        if blen < 12 or off + blen > len(data):
            break
        if btype == EPB:
            caplen = struct.unpack_from(endian + "I", data, off + 20)[0]
            yield data[off + 28 : off + 28 + caplen]
        off += blen


def strip_tap(pkt):
    """Remove the IEEE 802.15.4 TAP pseudo-header, returning the raw PSDU."""
    if len(pkt) < 4:
        return None
    _ver, _res, hdrlen = struct.unpack_from("<BBH", pkt, 0)
    if hdrlen < 4 or hdrlen > len(pkt):
        return None
    return pkt[hdrlen:]


def broadcast_dst_pan(psdu):
    """Rewrite the destination PAN ID to 0xFFFF.

    Captured frames carry the PAN ID of the network that produced them. The
    fuzz node generates a *random* dataset per run, so a captured PAN never
    matches and every frame is silently dropped by the destination-address
    filter (mac.cpp:1951) before reaching any parser. Broadcast PAN is
    accepted unconditionally, which is also what a real off-network attacker
    would send.
    """
    if len(psdu) < 5:
        return psdu
    fcf = struct.unpack_from("<H", psdu, 0)[0]
    dst_mode = (fcf >> 10) & 0x3
    if dst_mode == 0:  # no destination addressing -> no dst PAN present
        return psdu
    return psdu[:3] + b"\xff\xff" + psdu[5:]


def record(psdu, delay=1):
    psdu = broadcast_dst_pan(psdu)[:127]
    return bytes([len(psdu), delay]) + psdu


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    *inputs, outdir = sys.argv[1:]
    os.makedirs(outdir, exist_ok=True)

    frames = []
    for path in inputs:
        for pkt in parse_pcapng(path):
            psdu = strip_tap(pkt)
            if psdu and 0 < len(psdu) <= 127:
                frames.append(psdu)

    if not frames:
        print("NO FRAMES EXTRACTED -- check the capture", file=sys.stderr)
        return 2

    written = 0

    # One seed per individual frame, so the fuzzer has every distinct real
    # frame shape available as a mutation base.
    for i, f in enumerate(frames):
        blob = struct.pack("<I", i) + record(f)
        open(os.path.join(outdir, f"real_single_{i:04d}"), "wb").write(blob)
        written += 1

    # Sliding windows of consecutive frames, which is what actually exercises
    # multi-frame state (reassembly, block transfer, MLE exchanges).
    for width in (2, 3, 5, 8):
        for i in range(0, max(0, len(frames) - width + 1), max(1, width // 2)):
            window = frames[i : i + width]
            blob = struct.pack("<I", 0x1000 + i) + b"".join(record(f) for f in window)
            if len(blob) > 4 + 24 * (2 + 127):
                continue
            open(os.path.join(outdir, f"real_seq{width}_{i:04d}"), "wb").write(blob)
            written += 1

    sizes = sorted(set(len(f) for f in frames))
    print(f"extracted {len(frames)} frames, wrote {written} seeds to {outdir}/")
    print(f"frame lengths seen: min={sizes[0]} max={sizes[-1]} distinct={len(sizes)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
