#!/usr/bin/env python3
"""
Runner script for test_ext_network_diagnostic_win.py

Runs the test 4 * RUNS_PER_CONFIG times total (RUNS_PER_CONFIG runs per configuration combination):
  - EXTNETDIAG=True,  EVENTS=False
  - EXTNETDIAG=False, EVENTS=False
  - EXTNETDIAG=True,  EVENTS=True
  - EXTNETDIAG=False, EVENTS=True

For each run, an nRF 802.15.4 sniffer capture is started on COM20, channel 20,
with IEEE 802.15.4 TAP metadata. The pcap file is saved with the same name as
the log folder created by the test script.

Requirements:
  - Python 3.10+
  - pyserial
  - nrf802154_sniffer.py in C:\\Program Files\\Wireshark\\extcap\\
  - Sniffer dongle on COM20
  - Thread dongles on the COM ports specified in test_ext_network_diagnostic_win.py

Usage (from Windows command prompt or PowerShell):
  python run_all_tests.py
"""

import sys
import os
import time
import importlib
import importlib.util
import logging
import shutil
from datetime import datetime

SNIFFER_SCRIPT_DIR = r"C:\Program Files\Wireshark\extcap"
SNIFFER_PORT = "COM20"
SNIFFER_CHANNEL = 20
SNIFFER_METADATA = "ieee802154-tap"
PCAP_OUTPUT_DIR = "physical_test_logs"
RUNS_PER_CONFIG = 1
CONFIGURATIONS = [
    {
        "EXTNETDIAG": True,
        "EVENTS": False
    },
    {
        "EXTNETDIAG": False,
        "EVENTS": False
    },
    {
        "EXTNETDIAG": True,
        "EVENTS": True
    },
    {
        "EXTNETDIAG": False,
        "EVENTS": True
    },
]

# Add the Wireshark extcap directory to sys.path so we can import the sniffer
if SNIFFER_SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SNIFFER_SCRIPT_DIR)

# We import nrf802154_sniffer lazily after path setup
_sniffer_module = None


def get_sniffer_module():
    """Import the nrf802154_sniffer module (once)."""
    global _sniffer_module
    if _sniffer_module is None:
        _sniffer_module = importlib.import_module("nrf802154_sniffer")
    return _sniffer_module


def build_run_folder_name(extnetdiag: bool, events: bool) -> str:
    """
    Build the same folder name that test_ext_network_diagnostic_win.py will
    create for this configuration. Must match get_run_log_dir() logic.
    """
    protocol = "extnetdiag" if extnetdiag else "networkdiagnostics"
    events_str = "events_enabled" if events else "events_disabled"
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"{protocol}_{events_str}_{timestamp}"


def get_existing_log_dirs() -> set:
    """Return the set of existing subdirectory names under PCAP_OUTPUT_DIR."""
    if not os.path.isdir(PCAP_OUTPUT_DIR):
        return set()
    return {name for name in os.listdir(PCAP_OUTPUT_DIR) if os.path.isdir(os.path.join(PCAP_OUTPUT_DIR, name))}


def find_new_log_dir(before: set) -> str | None:
    """Find the log directory that was created since *before* snapshot."""
    after = get_existing_log_dirs()
    new_dirs = after - before
    if len(new_dirs) == 1:
        return new_dirs.pop()
    elif len(new_dirs) > 1:
        return sorted(new_dirs)[-1]
    return None


def start_sniffer(pcap_path: str):
    """
    Start the nRF 802.15.4 sniffer, writing pcap to *pcap_path*.

    Returns the sniffer instance (call sniffer.stop_thread() to stop).
    """
    mod = get_sniffer_module()
    sniffer = mod.Nrf802154Sniffer()

    print(f"  [sniffer] Starting capture -> {pcap_path}")
    print(f"  [sniffer] Port={SNIFFER_PORT}, Channel={SNIFFER_CHANNEL}, "
          f"Metadata={SNIFFER_METADATA}")

    sniffer.start_threaded(
        fifo=pcap_path,
        dev=SNIFFER_PORT,
        channel=SNIFFER_CHANNEL,
        metadata=SNIFFER_METADATA,
    )

    time.sleep(2)
    return sniffer


def stop_sniffer(sniffer):
    """Stop a running sniffer instance."""
    print("  [sniffer] Stopping capture...")
    try:
        sniffer.stop_thread()
    except Exception as e:
        print(f"  [sniffer] Warning during stop: {e}")
    time.sleep(1)


def patch_test_script(script_path: str, extnetdiag: bool, events: bool):
    """
    Rewrite the EXTNETDIAG and EVENTS constants in the test script in-place.
    Keeps a backup as .bak.
    """
    with open(script_path, "r", encoding="utf-8") as f:
        content = f.read()

    import re
    content = re.sub(
        r"^EXTNETDIAG\s*=\s*(True|False)",
        f"EXTNETDIAG = {extnetdiag}",
        content,
        count=1,
        flags=re.MULTILINE,
    )

    content = re.sub(
        r"^EVENTS\s*=\s*(True|False)",
        f"EVENTS = {events}",
        content,
        count=1,
        flags=re.MULTILINE,
    )

    with open(script_path, "w", encoding="utf-8") as f:
        f.write(content)


def reset_test_module_state():
    """
    The test script caches its log directory in module-level globals.
    We need to force a fresh import for every run so it picks up the new
    timestamp and configuration.
    """
    for mod_name in list(sys.modules.keys()):
        if "test_ext_network_diagnostic_win" in mod_name:
            del sys.modules[mod_name]

    for name in list(logging.Logger.manager.loggerDict.keys()):
        if name.startswith("thread_test"):
            logger = logging.getLogger(name)
            for handler in logger.handlers[:]:
                handler.close()
                logger.removeHandler(handler)


def _force_close_serial_ports():
    """
    Last-resort cleanup: close any serial.Serial instances that may still
    be open after a failed TestRunner initialization or run.
    """
    import gc
    import serial as pyserial

    print("  [cleanup] Force-closing any open serial ports...")
    gc.collect()
    for obj in gc.get_objects():
        try:
            if isinstance(obj, pyserial.Serial) and obj.is_open:
                print(f"  [cleanup] Closing {obj.port}")
                obj.close()
        except Exception:
            pass


def main():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    test_script = os.path.join(this_dir, "test_ext_network_diagnostic_win.py")

    if not os.path.isfile(test_script):
        print(f"ERROR: Test script not found: {test_script}")
        sys.exit(1)

    os.makedirs(PCAP_OUTPUT_DIR, exist_ok=True)

    backup_path = test_script + ".original_backup"
    if not os.path.exists(backup_path):
        shutil.copy2(test_script, backup_path)

    total_runs = len(CONFIGURATIONS) * RUNS_PER_CONFIG
    run_number = 0

    try:
        for config in CONFIGURATIONS:
            extnetdiag = config["EXTNETDIAG"]
            events = config["EVENTS"]

            for iteration in range(1, RUNS_PER_CONFIG + 1):
                run_number += 1
                print("=" * 72)
                print(f"RUN {run_number}/{total_runs}  "
                      f"EXTNETDIAG={extnetdiag}  EVENTS={events}  "
                      f"Iteration {iteration}/{RUNS_PER_CONFIG}")
                print("=" * 72)

                # 1. Patch the test script with current config
                patch_test_script(test_script, extnetdiag, events)

                # 2. Snapshot existing log dirs so we can detect the new one
                dirs_before = get_existing_log_dirs()

                # Use a temporary pcap name during capture; rename after
                # the test finishes so the name matches the actual log folder.
                predicted_name = build_run_folder_name(extnetdiag, events)
                temp_pcap_path = os.path.join(PCAP_OUTPUT_DIR, f"{predicted_name}_capture.pcap")

                print(f"  Expected log folder prefix: {predicted_name}")
                print(f"  Temp pcap file: {temp_pcap_path}")

                # 3. Start the sniffer
                sniffer = start_sniffer(temp_pcap_path)

                # 4. Run the test
                print("  [test] Starting test_ext_network_diagnostic_win.py ...")
                test_start = time.time()

                try:
                    reset_test_module_state()

                    # Change to test script directory for relative path consistency
                    old_cwd = os.getcwd()
                    os.chdir(this_dir)

                    # Import and run
                    spec = importlib.util.spec_from_file_location("test_ext_network_diagnostic_win", test_script)
                    test_module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(test_module)

                    runner = None
                    try:
                        runner = test_module.TestRunner()
                        runner.run()
                    finally:
                        # Always close serial ports, even if run() fails
                        if runner is not None and hasattr(runner, 'cleanup'):
                            runner.cleanup()
                        else:
                            # TestRunner() failed partway or has no cleanup — force close
                            _force_close_serial_ports()

                except Exception as e:
                    print(f"  [test] ERROR: {e}")
                    import traceback
                    traceback.print_exc()
                    _force_close_serial_ports()

                finally:
                    # Always restore working directory
                    try:
                        os.chdir(old_cwd)
                    except Exception:
                        pass

                    elapsed = time.time() - test_start
                    print(f"  [test] Finished in {elapsed / 60:.1f} minutes")

                    # 5. Stop the sniffer
                    stop_sniffer(sniffer)

                # 6. Rename pcap to match the actual log folder the test created
                actual_folder = find_new_log_dir(dirs_before)
                if actual_folder:
                    final_pcap = os.path.join(PCAP_OUTPUT_DIR, f"{actual_folder}.pcap")
                    try:
                        os.rename(temp_pcap_path, final_pcap)
                        print(f"  Pcap saved: {final_pcap}")
                    except OSError as e:
                        print(f"  Warning: could not rename pcap: {e}")
                        print(f"  Pcap saved as: {temp_pcap_path}")
                else:
                    print("  Warning: could not detect log folder, keeping temp name")
                    print(f"  Pcap saved as: {temp_pcap_path}")
                print()

                time.sleep(5)

    finally:
        # Restore the original test script
        if os.path.exists(backup_path):
            shutil.copy2(backup_path, test_script)
            os.remove(backup_path)
            print("Restored original test script.")

    print("=" * 72)
    print(f"ALL {total_runs} RUNS COMPLETE")
    print("=" * 72)


if __name__ == "__main__":
    main()
