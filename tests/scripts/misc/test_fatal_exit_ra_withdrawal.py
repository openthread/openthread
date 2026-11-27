#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
"""Verify POSIX ot-daemon withdraws BR prefixes on terminal RCP failure."""

import argparse
import ipaddress
import os
import select
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time

ICMP6_TYPE_ROUTER_ADVERT = 134
ND_OPT_PREFIX_INFORMATION = 3
ND_OPT_ROUTE_INFORMATION = 24

OT_EXIT_FAILURE = 1
OT_EXIT_ERROR_ERRNO = 5


def parse_router_advertisement(packet):
    """Parses an ICMPv6 Router Advertisement into PIO and RIO maps.

    Returns (pios, rios) where:
      - pios maps IPv6Network -> (valid_lifetime, preferred_lifetime)
      - rios maps IPv6Network -> route_lifetime
    Returns None if the packet is not a valid Router Advertisement.
    """
    if len(packet) < 16 or packet[0] != ICMP6_TYPE_ROUTER_ADVERT or packet[1] != 0:
        return None

    pios = {}
    rios = {}
    offset = 16

    while offset + 2 <= len(packet):
        opt_type = packet[offset]
        opt_len_units = packet[offset + 1]
        if opt_len_units == 0:
            return None

        opt_len = opt_len_units * 8
        if offset + opt_len > len(packet):
            return None

        opt = packet[offset:offset + opt_len]
        offset += opt_len

        if opt_type == ND_OPT_PREFIX_INFORMATION and opt_len == 32:
            prefix_len = opt[2]
            if prefix_len <= 128:
                valid_lft, preferred_lft = struct.unpack('!II', opt[4:12])
                prefix = ipaddress.IPv6Network(
                    (ipaddress.IPv6Address(opt[16:32]), prefix_len),
                    strict=False,
                )
                pios[prefix] = (valid_lft, preferred_lft)
        elif opt_type == ND_OPT_ROUTE_INFORMATION and opt_len in (8, 16, 24):
            prefix_len = opt[2]
            if prefix_len <= (opt_len - 8) * 8:
                route_lft = struct.unpack('!I', opt[4:8])[0]
                raw_prefix = opt[8:]
                padded_prefix = raw_prefix + b'\x00' * (16 - len(raw_prefix))
                prefix = ipaddress.IPv6Network(
                    (ipaddress.IPv6Address(padded_prefix), prefix_len),
                    strict=False,
                )
                rios[prefix] = route_lft

    if offset != len(packet):
        return None

    return pios, rios


def run_ot_ctl(ot_ctl, netif_name, *args, timeout=5.0):
    """Runs an ot-ctl command and returns stripped non-empty output lines."""
    proc = subprocess.run(
        [ot_ctl, '-I', netif_name, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding='utf-8',
        timeout=timeout,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f'ot-ctl {" ".join(args)} failed (rc={proc.returncode}): {proc.stdout}{proc.stderr}')
    lines = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
    if not lines or lines[-1] != 'Done':
        raise RuntimeError(f'ot-ctl {" ".join(args)} did not finish with Done: {lines}')
    return lines[:-1]


def wait_for_ot_ctl(ot_ctl, netif_name, args, expected=None, timeout=15.0):
    """Polls ot-ctl until it succeeds (and optionally matches expected text)."""
    deadline = time.monotonic() + timeout
    last_error = None
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        try:
            lines = run_ot_ctl(ot_ctl, netif_name, *args, timeout=min(5.0, max(0.1, remaining)))
            if expected is None or any(expected in line for line in lines):
                return lines
            last_error = f'unexpected output: {lines}'
        except (RuntimeError, subprocess.TimeoutExpired) as exc:
            last_error = str(exc)
        time.sleep(0.2)
    raise TimeoutError(f'Timed out waiting for ot-ctl {" ".join(args)} (expected={expected!r}): {last_error}')


def wait_for_link_local(if_name, timeout=10.0):
    """Waits until an interface has a non-tentative IPv6 link-local address."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        proc = subprocess.run(
            ['ip', '-6', '-o', 'addr', 'show', 'dev', if_name, 'scope', 'link'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding='utf-8',
            check=False,
        )
        for line in proc.stdout.splitlines():
            if 'inet6 fe80:' in line and 'tentative' not in line:
                return
        time.sleep(0.1)
    raise TimeoutError(f'Timed out waiting for non-tentative link-local address on {if_name}')


def find_child_pids(parent_pid):
    """Returns direct child PIDs of parent_pid by reading /proc."""
    children = []
    for entry in os.listdir('/proc'):
        if not entry.isdigit():
            continue
        pid = int(entry)
        try:
            with open(f'/proc/{pid}/stat', 'r', encoding='utf-8') as stat_file:
                stat_data = stat_file.read()
        except OSError:
            continue
        close_paren = stat_data.rfind(')')
        if close_paren == -1:
            continue
        fields = stat_data[close_paren + 2:].split()
        if len(fields) >= 2 and int(fields[1]) == parent_pid:
            children.append(pid)
    return children


def drain_socket(sock):
    """Drains any packets currently queued on sock."""
    while True:
        readable, _, _ = select.select([sock], [], [], 0.0)
        if not readable:
            break
        sock.recv(4096)


def wait_for_ra(sock, predicate, timeout):
    """Receives ICMPv6 packets until an RA satisfies predicate(pios, rios)."""
    deadline = time.monotonic() + timeout
    seen = []
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(f'Timed out waiting for expected RA; saw {len(seen)} RAs: {seen}')
        readable, _, _ = select.select([sock], [], [], remaining)
        if not readable:
            continue
        packet = sock.recv(4096)
        parsed = parse_router_advertisement(packet)
        if parsed is None:
            continue
        pios, rios = parsed
        seen.append((
            {
                str(k): v for k, v in pios.items()
            },
            {
                str(k): v for k, v in rios.items()
            },
        ))
        if predicate(pios, rios):
            return pios, rios


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ot-daemon', required=True, help='Path to ot-daemon executable')
    parser.add_argument('--ot-ctl', required=True, help='Path to ot-ctl executable')
    parser.add_argument('--ot-rcp', required=True, help='Path to ot-rcp executable')
    parser.add_argument('--netif-name', default='wpan-fatal', help='Thread interface name')
    parser.add_argument('--infra-if-br', default='veth-ot-br', help='BR infrastructure veth name')
    parser.add_argument('--infra-if-host', default='veth-ot-host', help='Peer host veth name')
    parser.add_argument('--rcp-node-id', default='2', help='Simulation node ID passed to ot-rcp')
    return parser.parse_args()


def main():
    args = parse_args()
    daemon = None
    rcp_pids = []
    failed = False

    with tempfile.TemporaryDirectory(prefix='ot-fatal-exit-ra-') as tmp_dir:
        daemon_log_path = os.path.join(tmp_dir, 'ot-daemon.log')
        data_path = os.path.join(tmp_dir, 'data')
        os.makedirs(data_path, exist_ok=True)

        try:
            subprocess.run(['ip', 'link', 'del', args.infra_if_br], stderr=subprocess.DEVNULL, check=False)
            subprocess.run(
                ['ip', 'link', 'add', args.infra_if_br, 'type', 'veth', 'peer', 'name', args.infra_if_host],
                check=True,
            )
            subprocess.run(['ip', 'link', 'set', args.infra_if_br, 'up'], check=True)
            subprocess.run(['ip', 'link', 'set', args.infra_if_host, 'up'], check=True)

            wait_for_link_local(args.infra_if_br)
            wait_for_link_local(args.infra_if_host)

            for suffix in ('.sock', '.lock'):
                sock_path = f'/run/openthread-{args.netif_name}{suffix}'
                if os.path.exists(sock_path):
                    os.unlink(sock_path)

            with socket.socket(socket.AF_INET6, socket.SOCK_RAW, socket.IPPROTO_ICMPV6) as ra_sock:
                ra_sock.setsockopt(
                    socket.SOL_SOCKET,
                    socket.SO_BINDTODEVICE,
                    args.infra_if_host.encode('ascii') + b'\x00',
                )

                radio_url = f'spinel+hdlc+forkpty://{args.ot_rcp}?forkpty-arg={args.rcp_node_id}'
                with open(daemon_log_path, 'w', encoding='utf-8') as log_file:
                    daemon = subprocess.Popen(
                        [
                            args.ot_daemon,
                            '-d5',
                            '-v',
                            '--data-path',
                            data_path,
                            '-I',
                            args.netif_name,
                            '-B',
                            args.infra_if_br,
                            radio_url,
                        ],
                        stdout=log_file,
                        stderr=subprocess.STDOUT,
                    )

                wait_for_ot_ctl(args.ot_ctl, args.netif_name, ['state'])

                rcp_pids = find_child_pids(daemon.pid)
                if len(rcp_pids) != 1:
                    raise RuntimeError(f'Expected 1 ot-rcp child of ot-daemon ({daemon.pid}), found {rcp_pids}')

                run_ot_ctl(args.ot_ctl, args.netif_name, 'dataset', 'init', 'new')
                run_ot_ctl(args.ot_ctl, args.netif_name, 'dataset', 'commit', 'active')
                run_ot_ctl(args.ot_ctl, args.netif_name, 'routerselectionjitter', '1')
                run_ot_ctl(args.ot_ctl, args.netif_name, 'ifconfig', 'up')
                run_ot_ctl(args.ot_ctl, args.netif_name, 'thread', 'start')

                wait_for_ot_ctl(args.ot_ctl, args.netif_name, ['state'], expected='leader', timeout=15.0)
                wait_for_ot_ctl(args.ot_ctl, args.netif_name, ['br', 'state'], expected='running', timeout=15.0)

                omr_lines = wait_for_ot_ctl(args.ot_ctl, args.netif_name, ['br', 'omrprefix', 'local'], timeout=15.0)
                onlink_lines = wait_for_ot_ctl(args.ot_ctl,
                                               args.netif_name, ['br', 'onlinkprefix', 'local'],
                                               timeout=15.0)
                local_omr = ipaddress.IPv6Network(omr_lines[0].split()[0], strict=False)
                local_onlink = ipaddress.IPv6Network(onlink_lines[0].split()[0], strict=False)

                wait_for_ra(
                    ra_sock,
                    lambda pios, rios:
                    (rios.get(local_omr, 0) > 0 and local_onlink in pios and pios[local_onlink][1] > 0),
                    timeout=30.0,
                )

                drain_socket(ra_sock)

                os.kill(rcp_pids[0], signal.SIGKILL)

                exit_code = daemon.wait(timeout=15.0)
                if exit_code not in (OT_EXIT_FAILURE, OT_EXIT_ERROR_ERRNO):
                    raise AssertionError(
                        f'Expected fatal ot-daemon exit code ({OT_EXIT_FAILURE} or {OT_EXIT_ERROR_ERRNO}), '
                        f'got {exit_code}')

                pios, rios = wait_for_ra(
                    ra_sock,
                    lambda p, r:
                    (local_omr in r and r[local_omr] == 0 and local_onlink in p and p[local_onlink][1] == 0),
                    timeout=5.0,
                )

                print(f'Verified fatal-exit RA withdrawal: exit_code={exit_code}, '
                      f'omr_rio={local_omr} (lft={rios[local_omr]}), '
                      f'onlink_pio={local_onlink} (valid={pios[local_onlink][0]}, pref={pios[local_onlink][1]})')
        except Exception:
            failed = True
            raise
        finally:
            for pid in rcp_pids:
                try:
                    os.kill(pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

            if daemon is not None and daemon.poll() is None:
                daemon.terminate()
                try:
                    daemon.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    daemon.kill()
                    daemon.wait()

            for suffix in ('.sock', '.lock'):
                sock_path = f'/run/openthread-{args.netif_name}{suffix}'
                try:
                    os.unlink(sock_path)
                except FileNotFoundError:
                    pass

            subprocess.run(['ip', 'link', 'del', args.infra_if_br], stderr=subprocess.DEVNULL, check=False)

            if failed and os.path.exists(daemon_log_path):
                sys.stderr.write('--- ot-daemon log ---\n')
                with open(daemon_log_path, 'r', encoding='utf-8', errors='replace') as log_file:
                    sys.stderr.write(log_file.read())

    return 0


if __name__ == '__main__':
    sys.exit(main())
