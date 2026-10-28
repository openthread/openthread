"""
  Copyright (c) 2026, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

# Buffer sizes (bytes)
TLS_BUFFER_SIZE = 4096  # max bytes read from / written to the TLS and BLE/UDP link per call

# Timeouts (seconds)
TLS_HANDSHAKE_TIMEOUT_BLE_SEC = 30.0  # TLS handshake with a real TCAT Device over BLE
TLS_HANDSHAKE_TIMEOUT_SIMULATION_SEC = 5.0  # TLS handshake with a simulated TCAT Device over UDP
TLS_CLOSE_TIMEOUT_SEC = 5.0  # graceful TLS close (close-notify exchange)
TCAT_RESPONSE_TIMEOUT_SEC = 5.0  # waiting for the response to a TCAT command
BLE_RX_QUIET_TIME_SEC = 0.200  # BLE data counts as complete after no new notification for this long

# Polling intervals (seconds)
POLL_INTERVAL_SEC = 0.020  # sleep between link reads while waiting for data
EVENT_POLL_INTERVAL_SEC = 0.100  # background receive loop interval for unsolicited events

# Simulation
SIMULATION_HOST = '127.0.0.1'  # host of the simulated TCAT Device (UDP)
