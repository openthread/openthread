import serial
import time
import random
import logging
from logging.handlers import RotatingFileHandler
import re
import os
from datetime import datetime

# This script will run one instance of the extended network diagnostic protocol test.
# Configuration can be done using the constants below.
#
# Wireshark captures need to be started and stopped externally.
#
# The dongles use the specified COM ports in FTD_PORTS and MTD_PORTS lists.
#
# The leader dongle must support network diagnostic, mesh diagnostic and extended network diagnostic.
# All dongles must support the mac filter and CSL receiver
#
# The dongles will be automatically reset at the start of the test and stopped at the end.

# If true, the extended network diagnostic protocol will be used. If false, diagnostic query.
EXTNETDIAG = True
# If true, reattach events will be triggered during the test.
EVENTS = True

THREAD_DATASET = "0e0800000000000100004a0300001835060004001fffe00208eccb32894ac56ea10708fdf177e93ad6f81d0510e8ceb3189b5944d425e408b064c8fd2d030f4f70656e5468726561642d31336131010213a1041037806c9af95a5073800afd03efaf24360c0402a0f7f80003000014"
QUERY_INTERVAL = 60 * 5

FTD_PORTS = ["COM14", "COM7", "COM15", "COM3", "COM33"]
MTD_PORTS = ["COM12", "COM13", "COM9", "COM4", "COM5", "COM24", "COM26", "COM28", "COM30"]

# Logging setup
LOG_BASE_DIR = "physical_test_logs"
LOG_FILE = "thread_test.log"
LOG_MAX_BYTES = 10 * 1024 * 1024  # 10 MB
LOG_BACKUP_COUNT = 5

# How often to refresh RLOC16 for all devices (seconds)
RLOC16_REFRESH_INTERVAL = 60 * 5

_main_logger = None
_port_loggers = {}
_log_dir = None


def get_run_log_dir():
    """Generate unique log directory name for this run based on config and timestamp."""
    global _log_dir
    if _log_dir is not None:
        return _log_dir

    # Build directory name components
    protocol = "extnetdiag" if EXTNETDIAG else "networkdiagnostics"
    events = "events_enabled" if EVENTS else "events_disabled"
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    # Create unique directory name
    run_dir_name = f"{protocol}_{events}_{timestamp}"
    _log_dir = os.path.join(LOG_BASE_DIR, run_dir_name)

    # Create the directory
    os.makedirs(_log_dir, exist_ok=True)

    return _log_dir


def setup_logging():
    """Setup structured logging to main file with rotation."""
    global _main_logger
    if _main_logger is not None:
        return _main_logger

    # Get the run-specific log directory
    log_dir = get_run_log_dir()

    _main_logger = logging.getLogger("thread_test_main")
    _main_logger.setLevel(logging.DEBUG)

    # Avoid duplicate handlers if module is reloaded
    if _main_logger.handlers:
        return _main_logger

    # Main combined log file
    handler = RotatingFileHandler(
        os.path.join(log_dir, LOG_FILE),
        maxBytes=LOG_MAX_BYTES,
        backupCount=LOG_BACKUP_COUNT,
        encoding="utf-8",
    )
    formatter = logging.Formatter(
        "%(asctime)s.%(msecs)03d | %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    handler.setFormatter(formatter)
    _main_logger.addHandler(handler)

    return _main_logger


def get_port_logger(port):
    """Get or create a per-port logger."""
    global _port_loggers

    if port in _port_loggers:
        return _port_loggers[port]

    # Get the run-specific log directory
    log_dir = get_run_log_dir()

    # Sanitize port name for filename (e.g., COM14 -> COM14.log)
    safe_port = port.replace("/", "_").replace("\\", "_").replace(":", "")
    log_file = os.path.join(log_dir, f"{safe_port}.log")

    logger = logging.getLogger(f"thread_test_{safe_port}")
    logger.setLevel(logging.DEBUG)

    # Avoid duplicate handlers
    if not logger.handlers:
        handler = RotatingFileHandler(
            log_file,
            maxBytes=LOG_MAX_BYTES,
            backupCount=LOG_BACKUP_COUNT,
            encoding="utf-8",
        )
        formatter = logging.Formatter(
            "%(asctime)s.%(msecs)03d | %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
        handler.setFormatter(formatter)
        logger.addHandler(handler)

    _port_loggers[port] = logger
    return logger


def get_main_logger():
    """Get the main combined logger, initializing if needed."""
    global _main_logger
    if _main_logger is None:
        setup_logging()
    return _main_logger


class Device:

    def __init__(self, port, role="unknown"):
        self.port = port
        self.role = role  # "leader", "ftd", "mtd_rxon", "mtd_rxoff"
        self.serial = serial.Serial(port, 19200, timeout=60, write_timeout=60)
        self.extaddr = "unknown"
        self.rloc16 = "unknown"
        self.state = "unknown"
        self.last_rloc16_update = 0

        # Dual logging: per-port and main combined
        self.log_main = get_main_logger()
        self.log_port = get_port_logger(port)

        # Initial extaddr fetch
        self._drain_input()
        self.serial.write(b'extaddr\n')
        self.serial.readline()
        self.extaddr = self.serial.readline().decode('utf-8').rstrip()
        self._log_both("INIT", f"Device initialized, extaddr={self.extaddr}, role={self.role}")

    def tag(self):
        """Return a device identifier string for logging."""
        return f"{self.port} ext={self.extaddr} rloc16={self.rloc16}"

    def short_tag(self):
        """Return a shorter tag for per-port logs (port info is redundant there)."""
        return f"ext={self.extaddr} rloc16={self.rloc16} state={self.state}"

    def _log_both(self, level, message):
        """Log to both main and per-port log files."""
        self.log_main.info(f"{self.tag()} | {level} | {message}")
        self.log_port.info(f"{self.short_tag()} | {level} | {message}")

    def _log_tx(self, command):
        """Log transmitted command to both logs."""
        self.log_main.info(f"{self.tag()} | TX | {command}")
        self.log_port.info(f"{self.short_tag()} | TX | {command}")

    def _log_rx(self, line):
        """Log received line to both logs."""
        self.log_main.info(f"{self.tag()} | RX | {line}")
        self.log_port.info(f"{self.short_tag()} | RX | {line}")

    def _log_rx_block(self, lines, prefix=""):
        """Log multiple received lines to both logs."""
        for line in lines:
            display = f"{prefix}{line}" if prefix else line
            self.log_main.info(f"{self.tag()} | RX | {display}")
            self.log_port.info(f"{self.short_tag()} | RX | {display}")

    def _drain_input(self):
        """Drain any pending input from the serial buffer."""
        self.serial.reset_input_buffer()
        # Give a moment and drain again in case of late data
        time.sleep(0.05)
        while self.serial.in_waiting > 0:
            self.serial.read(self.serial.in_waiting)
            time.sleep(0.01)

    def _read_response(self, timeout=5.0, quiet_period=0.3):
        """
        Read response from serial until we see 'Done' followed by prompt,
        or until timeout with a quiet period (no new data).

        Handles:
        - Commands echoed back
        - "Done" followed by "> " prompt
        - Doubled prompts "> >"
        - Partial/garbled output
        """
        start_time = time.time()
        last_data_time = time.time()
        buffer = b""

        original_timeout = self.serial.timeout
        self.serial.timeout = 0.1  # Short timeout for non-blocking reads

        try:
            while (time.time() - start_time) < timeout:
                chunk = self.serial.read(256)
                if chunk:
                    buffer += chunk
                    last_data_time = time.time()
                    continue

                # No data received, check quiet period
                quiet_elapsed = time.time() - last_data_time
                if quiet_elapsed > quiet_period:
                    decoded = buffer.decode('utf-8', errors='replace')
                    if 'Done' in decoded or 'Error' in decoded or '> ' in decoded:
                        break
                    if quiet_elapsed > quiet_period * 2:
                        break
        finally:
            self.serial.timeout = original_timeout

        return self._parse_response_buffer(buffer)

    def _parse_response_buffer(self, buffer):
        """Parse raw buffer bytes into cleaned response lines."""
        decoded = buffer.decode('utf-8', errors='replace')
        lines = decoded.replace('\r\n', '\n').replace('\r', '\n').split('\n')
        response_lines = []
        for line in lines:
            cleaned = line.strip()
            cleaned = re.sub(r'^>\s*>?\s*', '', cleaned)  # Remove leading prompts
            cleaned = re.sub(r'\s*>\s*$', '', cleaned)  # Remove trailing prompts
            if cleaned:
                response_lines.append(cleaned)
        return response_lines

    def _cmd(self, command, timeout=5.0, quiet_period=0.3, log_tx=True, log_rx=True):
        """
        Send a command and read the response with full logging.

        Args:
            command: The command string to send (without newline)
            timeout: Maximum time to wait for response
            quiet_period: Time to wait after last data before considering response complete
            log_tx: Whether to log the transmitted command
            log_rx: Whether to log the received response

        Returns:
            List of response lines (cleaned)
        """
        # Drain any pending input
        self._drain_input()

        # Send command
        cmd_bytes = (command + '\n').encode('utf-8')
        if log_tx:
            self._log_tx(command)

        self.serial.write(cmd_bytes)

        # Read response
        response_lines = self._read_response(timeout=timeout, quiet_period=quiet_period)

        if log_rx and response_lines:
            # Filter out echo of the command itself
            filtered_lines = [line for line in response_lines if line != command and not line.endswith(command)]
            self._log_rx_block(filtered_lines)

        return response_lines

    def _cmd_no_response(self, command):
        """
        Send a command where we don't strictly need the response for logic,
        but still log it. Uses shorter timeout.
        """
        return self._cmd(command, timeout=2.0, quiet_period=0.2)

    def _cmd_long(self, command, timeout=15.0, quiet_period=1.0):
        """
        Send a command that may have long/multi-line response (e.g., diagnostics).
        """
        return self._cmd(command, timeout=timeout, quiet_period=quiet_period)

    def process(self):
        self.serial.reset_input_buffer()

    def _parse_state_response(self, response):
        """Extract state from response lines."""
        valid_states = ('disabled', 'detached', 'child', 'router', 'leader')
        for line in response:
            if line in valid_states:
                return line
        return None

    def _parse_rloc16_response(self, response):
        """Extract rloc16 from response lines."""
        for line in response:
            if line and line != 'rloc16' and line != 'Done' and not line.startswith('Error'):
                return line
        return None

    def update_state(self, force=False):
        """Update device state (state + rloc16). Called periodically or on-demand."""
        now = time.time()

        # Skip if we already have values and it's not time to refresh yet
        if not force and self.state != "unknown" and self.rloc16 != "unknown":
            if now - self.last_rloc16_update < RLOC16_REFRESH_INTERVAL:
                return

        # Also skip if recently updated (unless forced)
        if not force and now - self.last_rloc16_update < RLOC16_REFRESH_INTERVAL:
            return

        self.last_rloc16_update = now
        self._update_thread_state()
        self._update_rloc16()

    def _update_thread_state(self):
        """Query and update thread state."""
        response = self._cmd('state', timeout=2.0, log_rx=False)
        new_state = self._parse_state_response(response)
        if new_state and self.state != new_state:
            old_state = self.state
            self.state = new_state
            self._log_both("STATE", f"State changed: {old_state} -> {self.state}")

    def _update_rloc16(self):
        """Query and update rloc16 if device is attached."""
        if self.state not in ('child', 'router', 'leader'):
            if self.rloc16 != "unknown":
                self.rloc16 = "unknown"
                self._log_both("RLOC16", "RLOC16 reset to unknown (not attached)")
            return

        response = self._cmd('rloc16', timeout=2.0, log_rx=False)
        new_rloc = self._parse_rloc16_response(response)
        if new_rloc and self.rloc16 != new_rloc:
            old_rloc = self.rloc16
            self.rloc16 = new_rloc
            self._log_both("RLOC16", f"RLOC16 changed: {old_rloc} -> {self.rloc16}")

    def reset(self):
        self._log_both("ACTION", "reset starting")
        self._cmd_no_response('ifconfig down')
        self.serial.write(b'factoryreset\n')
        self._log_tx('factoryreset')
        time.sleep(5)
        self.serial.reset_input_buffer()

        # Re-fetch extaddr after reset
        response = self._cmd('extaddr', timeout=5.0)
        for line in response:
            if line and line != 'extaddr' and line != 'Done':
                self.extaddr = line
                break
        self.rloc16 = "unknown"
        self.state = "disabled"
        self.last_rloc16_update = 0
        self._log_both("ACTION", f"reset complete, extaddr={self.extaddr}")

        self._cmd_no_response(f"dataset set active {THREAD_DATASET}")

    def add_deny(self, device):
        self._log_both("ACTION", f"add_deny for {device.extaddr}")
        self._cmd_no_response(f"macfilter addr add {device.extaddr}")
        self._cmd_no_response('macfilter addr denylist')

    def add_allow(self, device):
        self._log_both("ACTION", f"add_allow for {device.extaddr}")
        self._cmd_no_response(f"macfilter addr add {device.extaddr}")
        self._cmd_no_response('macfilter addr allowlist')

    def config_ftd(self):
        self._log_both("ACTION", "config_ftd (mode rdn)")
        self._cmd_no_response('mode rdn')

    def config_mtd_rxon(self):
        self._log_both("ACTION", "config_mtd_rxon (mode r)")
        self._cmd_no_response('mode r')

    def config_mtd_rxoff(self):
        self._log_both("ACTION", "config_mtd_rxoff (mode -, pollperiod 120000)")
        self._cmd_no_response('mode -')
        self._cmd_no_response('pollperiod 120000')

    def set_csl(self, period):
        self._log_both("ACTION", f"set_csl period={period}")
        self._cmd_no_response(f"csl period {period}")

    def start(self):
        self._log_both("ACTION", "start (ifconfig up, thread start)")
        self._cmd_no_response('ifconfig up')
        self._cmd_no_response('thread start')
        self.state = "detached"
        self.last_rloc16_update = 0  # Force refresh on next update

    def stop(self):
        self._log_both("ACTION", "stop (thread stop)")
        self._cmd_no_response('thread stop')
        self.state = "disabled"
        self.rloc16 = "unknown"

    def close(self):
        """Close the serial port, releasing the handle."""
        self._log_both("ACTION", "closing serial port")
        try:
            if self.serial and self.serial.is_open:
                self.serial.close()
        except Exception as e:
            self._log_both("WARNING", f"Error closing serial port: {e}")

    def configure_extnetdiag(self):
        self._log_both("ACTION", "configure_extnetdiag - setting TLV configs")

        # Host TLVs
        self._log_both("CONFIG", "extnetdiag host TLVs: 0 6 24 26")
        self._cmd_no_response('extnetdiag config -h 0 6 24 26')

        # Child TLVs
        self._log_both("CONFIG", "extnetdiag child TLVs: 0 1 2 3 4 5 7 8 24 25 26 27")
        self._cmd_no_response('extnetdiag config -c 0 1 2 3 4 5 7 8 24 25 26 27')

        # Neighbor TLVs
        self._log_both("CONFIG", "extnetdiag neighbor TLVs: 0 3 4 7 8 27")
        self._cmd_no_response('extnetdiag config -n 0 3 4 7 8 27')

        # Start the protocol
        self._log_both("CONFIG", "extnetdiag start")
        self._cmd_no_response('extnetdiag start')

    def is_router_or_leader(self):
        """Check if device is router or leader. Updates state only if unknown."""
        if self.state == "unknown":
            self.update_state(force=True)
        return self.state in ('router', 'leader')

    def get_rloc16(self):
        """Get cached rloc16, fetching only if unknown."""
        if self.rloc16 == "unknown":
            self.update_state(force=True)
        return self.rloc16

    def send_mesh_diag(self, target):
        """Send mesh diagnostic queries to a target router."""
        self._log_both("DIAG", f"send_mesh_diag target=0x{target}")

        # meshdiag childtable
        cmd1 = f"meshdiag childtable 0x{target}"
        self._log_both("DIAG", f"Querying child table for router 0x{target}")
        self._cmd_long(cmd1, timeout=10.0, quiet_period=1.0)

        # meshdiag routerneighbortable
        time.sleep(0.5)
        cmd2 = f"meshdiag routerneighbortable 0x{target}"
        self._log_both("DIAG", f"Querying router neighbor table for 0x{target}")
        self._cmd_long(cmd2, timeout=10.0, quiet_period=1.0)

    def send_net_diag(self):
        """Send network diagnostic query to all nodes."""
        self._log_both("DIAG", "send_net_diag to ff03::1")

        cmd = 'networkdiagnostic get ff03::1 0 2 3 4 5 9 34'
        self._log_both(
            "DIAG", "Requesting TLVs: 0(MAC Extended Address) 2(Mode) 3(Timeout) 4(Connectivity) "
            "5(Route64) 9(MAC Counters) 34(MLE Counters)")
        self._cmd_long(cmd, timeout=15.0, quiet_period=2.0)

    def send_diag(self, devices):
        """Send all diagnostic queries."""
        self._log_both("DIAG", "========== Starting diagnostic query cycle ==========")

        # Query mesh diag for each router
        routers = []
        for d in devices:
            d.update_state()
            if d.state in ('router', 'leader'):
                routers.append(d)

        self._log_both("DIAG", f"Found {len(routers)} routers to query")
        for d in routers:
            self._log_both("DIAG", f"  Router: {d.port} rloc16={d.rloc16} ext={d.extaddr}")

        for d in routers:
            self.send_mesh_diag(d.rloc16)
            time.sleep(1)

        # Send network diagnostic
        self.send_net_diag()
        self._log_both("DIAG", "========== Diagnostic query cycle complete ==========")


class TestRunner:

    def __init__(self):
        self.log = get_main_logger()
        self.log.info("=" * 80)
        self.log.info("TestRunner initializing")
        self.log.info(f"EXTNETDIAG={EXTNETDIAG}, EVENTS={EVENTS}")
        self.log.info(f"RLOC16_REFRESH_INTERVAL={RLOC16_REFRESH_INTERVAL}s")
        self.log.info(f"FTD_PORTS={FTD_PORTS}")
        self.log.info(f"MTD_PORTS={MTD_PORTS}")
        self.log.info("=" * 80)

        self.devices = []
        self.leader = Device(FTD_PORTS[0], role="leader")
        self.leader.reset()
        self.leader.config_ftd()
        self.rng = random.Random(17)

        for i in range(1, 5):
            dev = Device(FTD_PORTS[i], role="ftd")
            dev.reset()
            dev.config_ftd()
            self.devices.append(dev)

        self.devices[0].add_deny(self.leader)
        self.devices[1].add_deny(self.leader)

        for i in range(0, 4):
            dev = Device(MTD_PORTS[i], role="mtd_rxoff_csl")
            dev.reset()
            dev.config_mtd_rxoff()
            dev.set_csl(5000000)
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

        for i in range(4, 6):
            dev = Device(MTD_PORTS[i], role="mtd_rxoff")
            dev.reset()
            dev.config_mtd_rxoff()
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

        for i in range(6, 9):
            dev = Device(MTD_PORTS[i], role="mtd_rxon")
            dev.reset()
            dev.config_mtd_rxon()
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

        self.log.info("TestRunner initialization complete")
        self._log_device_summary()

    def _log_device_summary(self):
        """Log summary of all devices."""
        self.log.info("=" * 80)
        self.log.info("DEVICE SUMMARY:")
        self.log.info(f"  Leader: {self.leader.port} ext={self.leader.extaddr}")
        for i, d in enumerate(self.devices):
            self.log.info(f"  Device[{i}]: {d.port} ext={d.extaddr} role={d.role}")
        self.log.info("=" * 80)

    def _update_all_device_states(self, force=False):
        """Update state/rloc16 for all devices."""
        self.leader.update_state(force=force)
        for d in self.devices:
            d.update_state(force=force)

    def _log_network_topology(self):
        """Log current network topology."""
        self.log.info("-" * 60)
        self.log.info("NETWORK TOPOLOGY:")
        self.log.info(f"  Leader: {self.leader.port} rloc16={self.leader.rloc16} state={self.leader.state}")

        routers = [d for d in self.devices if d.state == 'router']
        children = [d for d in self.devices if d.state == 'child']
        detached = [d for d in self.devices if d.state in ('detached', 'disabled')]

        self.log.info(f"  Routers ({len(routers)}):")
        for d in routers:
            self.log.info(f"    {d.port} rloc16={d.rloc16} ext={d.extaddr} role={d.role}")

        self.log.info(f"  Children ({len(children)}):")
        for d in children:
            self.log.info(f"    {d.port} rloc16={d.rloc16} ext={d.extaddr} role={d.role}")

        if detached:
            self.log.info(f"  Detached/Disabled ({len(detached)}):")
            for d in detached:
                self.log.info(f"    {d.port} state={d.state} role={d.role}")

        self.log.info("-" * 60)

    def run(self):
        print("Starting Leader")
        self.log.info("Starting Leader")
        self.leader.start()
        time.sleep(30)

        print("Starting Devices")
        self.log.info("Starting Devices")
        for d in self.devices:
            d.start()

        self.log.info("Waiting 120s for network formation...")
        time.sleep(120)

        # Update all device states after formation
        self._update_all_device_states()
        self._log_network_topology()

        if EXTNETDIAG:
            print("Starting extended network diagnostic protocol")
            self.log.info("Starting extended network diagnostic protocol")
            self.leader.configure_extnetdiag()

        print("All devices ready. Starting test...")
        self.log.info("All devices ready. Starting test...")
        if not EXTNETDIAG:
            self.leader.send_diag(self.devices)

        self.start_time = time.time()
        self.query_time = time.time()

        if not EVENTS:
            self.__loop_until(63)
        else:
            self.__loop_until(12)
            self.__restart_random(3)
            self.__loop_until(22)
            self.__restart_children()
            self.__loop_until(32)
            self.__restart_random(3)
            self.__loop_until(42)
            self.__restart_children()
            self.__loop_until(52)
            self.__restart_random(3)
            self.__loop_until(63)

        print("Test complete. Stopping devices...")
        self.log.info("Test complete. Stopping devices...")

        for d in self.devices:
            d.stop()
        self.leader.stop()
        print("Done")
        self.log.info("Done")

    def cleanup(self):
        """Close all serial ports so they can be reused in subsequent runs."""
        self.log.info("Cleanup: closing all serial ports")
        for d in self.devices:
            d.close()
        self.leader.close()
        self.log.info("Cleanup complete")

    def __restart_children(self):
        self.log.info("=" * 60)
        self.log.info("EVENT | restart_children - Stopping all children")

        # Update states first
        self._update_all_device_states()

        children = [d for d in self.devices if d.state == 'child']
        self.log.info(f"EVENT | Found {len(children)} children to restart:")
        for d in children:
            self.log.info(f"EVENT |   {d.port} rloc16={d.rloc16} ext={d.extaddr}")
            d.stop()

        self.log.info("EVENT | Waiting 60s before restarting children...")
        time.sleep(60)

        self.log.info("EVENT | Restarting children...")
        for d in children:
            d.start()

        self.log.info("EVENT | restart_children complete, waiting for rejoin...")
        time.sleep(30)  # Give time to rejoin

        # Update and log new topology
        self._update_all_device_states()
        self._log_network_topology()
        self.log.info("=" * 60)

    def __restart_random(self, n):
        self.log.info("=" * 60)
        self.log.info(f"EVENT | restart_random n={n}")

        # Update states first
        self._update_all_device_states()

        selected = self.rng.sample(self.devices, n)
        self.log.info(f"EVENT | Selected {n} random devices to restart:")
        for d in selected:
            self.log.info(f"EVENT |   {d.port} rloc16={d.rloc16} state={d.state} role={d.role}")
            d.stop()

        self.log.info("EVENT | Waiting 60s before restarting...")
        time.sleep(60)

        self.log.info("EVENT | Restarting selected devices...")
        for d in selected:
            d.start()

        self.log.info("EVENT | restart_random complete, waiting for rejoin...")
        time.sleep(30)  # Give time to rejoin

        # Update and log new topology
        self._update_all_device_states()
        self._log_network_topology()
        self.log.info("=" * 60)

    def __loop_until(self, end):
        self.log.info(f"LOOP | Starting loop until {end}min")
        while time.time() < self.start_time + (end * 60) + 12:
            if time.time() >= self.query_time:
                t = (time.time() - self.start_time) / 60
                print(f"Checkpoint: {t:.1f}min")
                self.log.info(f"CHECKPOINT | {t:.2f}min elapsed")

                # Update all device states at checkpoints only
                self._update_all_device_states(force=True)
                self._log_network_topology()

                self.query_time += QUERY_INTERVAL
                if not EXTNETDIAG:
                    self.leader.send_diag(self.devices)

            time.sleep(10)

            self.leader.process()
            for d in self.devices:
                d.process()


# Initialize logging at module load
setup_logging()

if __name__ == "__main__":
    runner = TestRunner()
    runner.run()
