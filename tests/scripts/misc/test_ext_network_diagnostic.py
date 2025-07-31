import serial
import time
import random

# This script will run one instance of the diagnostic protocol test.
# Configuration can be done using the constants below.
#
# Wireshark captures need to be started and stopped externally.
#
# The dongles use the 14 serial addresses /dev/ttyACM{SERIAL_BASE} to /dev/ttyACM{SERIAL_BASE + 13}
#
# The leader dongle must support network diagnostic, mesh diagnostic and the diagnostic server.
# All dongles must support the mac filter and CSL receiver
#
# The dongles will be automatically reset at the start of the test and stopped at the end.

# If true the diagnostic server protocol will be used. If false diagnostic query
DIAG_SERVER = True
# If true reattach events will be triggered during the test.
EVENTS = True

THREAD_DATASET = "0e0800000000000100004a0300001835060004001fffe00208eccb32894ac56ea10708fdf177e93ad6f81d0510e8ceb3189b5944d425e408b064c8fd2d030f4f70656e5468726561642d31336131010213a1041037806c9af95a5073800afd03efaf24360c0402a0f7f80003000014"
SERIAL_BASE = 0
QUERY_INTERVAL = 60 * 5


class Device:

    def __init__(self, port):
        self.serial = serial.Serial(port, 19200, timeout=60, write_timeout=60)
        self.serial.write(b'extaddr\n')
        self.serial.readline()
        self.extaddr = self.serial.readline().decode('utf-8').rstrip()

    def process(self):
        self.serial.reset_input_buffer()

    def reset(self):
        self.serial.write(b'\nifconfig down\n')
        self.serial.write(b'factoryreset\n')
        time.sleep(5)
        self.serial.reset_input_buffer()
        self.serial.write(b'extaddr\n')
        self.serial.readline()
        self.extaddr = self.serial.readline().decode('utf-8').rstrip()
        self.serial.write(f"dataset set active {THREAD_DATASET}\n".encode('utf-8'))

    def add_deny(self, device):
        self.serial.write(f"macfilter addr add {device.extaddr}\n".encode('utf-8'))
        self.serial.write(b'macfilter addr denylist\n')

    def add_allow(self, device):
        self.serial.write(f"macfilter addr add {device.extaddr}\n".encode('utf-8'))
        self.serial.write(b'macfilter addr allowlist\n')

    def config_ftd(self):
        self.serial.write(b'mode rdn\n')

    def config_mtd_rxon(self):
        self.serial.write(b'mode r\n')

    def config_mtd_rxoff(self):
        self.serial.write(b'mode -\n')
        self.serial.write(b'pollperiod 120000\n')

    def set_csl(self, period):
        self.serial.write(f"csl period {period}\n".encode('utf-8'))

    def start(self):
        self.serial.write(b'ifconfig up\n')
        self.serial.write(b'thread start\n')

    def stop(self):
        self.serial.write(b'thread stop\n')

    def configure_diag_client(self):
        self.serial.write(b'extnetdiag config -h 16 17 18 19 20 26\n')
        self.serial.write(b'extnetdiag config -c 0 1 2 3 4 5 7 8 16 17 18 19 20 26\n')
        self.serial.write(b'extnetdiag config -n 0 4 7 8 16\n')
        self.serial.write(b'extnetdiag start\n')

    def is_router_or_leader(self):
        time.sleep(0.5)
        self.serial.reset_input_buffer()
        self.serial.write(b'state\n')
        self.serial.readline()
        state = self.serial.readline().decode('utf-8').rstrip()
        return (state == "router") or (state == "leader")

    def get_rloc16(self):
        time.sleep(0.5)
        self.serial.reset_input_buffer()
        self.serial.write(b'rloc16\n')
        self.serial.readline()
        return self.serial.readline().decode('utf-8').rstrip()

    def send_mesh_diag(self, target):
        time.sleep(0.5)
        self.serial.reset_input_buffer()
        self.serial.write(f"meshdiag childtable 0x{target}\n".encode('utf-8'))
        time.sleep(5)
        self.serial.write(f"meshdiag routerneighbortable 0x{target}\n".encode('utf-8'))
        time.sleep(5)

    def send_net_diag(self):
        time.sleep(0.5)
        self.serial.reset_input_buffer()
        self.serial.write(b'networkdiagnostic get ff03::1 25 26 27 28 34\n')
        time.sleep(10)

    def send_diag(self, devices):
        for d in devices:
            if d.is_router_or_leader():
                self.send_mesh_diag(d.get_rloc16())
        self.send_net_diag()


class TestRunner:

    def __init__(self):
        self.devices = []
        self.leader = Device(f"/dev/ttyACM{SERIAL_BASE}")
        self.leader.reset()
        self.leader.config_ftd()
        self.rng = random.Random(17)

        serial = SERIAL_BASE + 1
        for i in range(serial, serial + 4):
            dev = Device(f"/dev/ttyACM{i}")
            dev.reset()
            dev.config_ftd()
            self.devices.append(dev)

        self.devices[0].add_deny(self.leader)
        self.devices[1].add_deny(self.leader)

        serial += 4
        for i in range(serial, serial + 4):
            dev = Device(f"/dev/ttyACM{i}")
            dev.reset()
            dev.config_mtd_rxoff()
            dev.set_csl(5000000)
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

        serial += 4
        for i in range(serial, serial + 2):
            dev = Device(f"/dev/ttyACM{i}")
            dev.reset()
            dev.config_mtd_rxoff()
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

        serial += 2
        for i in range(serial, serial + 3):
            dev = Device(f"/dev/ttyACM{i}")
            dev.reset()
            dev.config_mtd_rxon()
            dev.add_allow(self.devices[self.rng.randrange(0, 4)])
            self.devices.append(dev)

    def run(self):
        print("Starting Leader")
        self.leader.start()
        time.sleep(30)

        print("Starting Devices")
        for d in self.devices:
            d.start()

        time.sleep(120)

        if DIAG_SERVER:
            print("Starting diag server")
            self.leader.configure_diag_client()

        print("All devices ready. Starting test...")
        if not DIAG_SERVER:
            self.leader.send_diag(self.devices)

        self.start_time = time.time()
        self.query_time = time.time()

        if not EVENTS:
            self.__loop_until(63)
        else:
            self.__loop_until(10)
            self.__restart_random(3)
            self.__loop_until(20)
            self.__restart_children()
            self.__loop_until(30)
            self.__restart_random(3)
            self.__loop_until(40)
            self.__restart_children()
            self.__loop_until(50)
            self.__restart_random(3)
            self.__loop_until(63)

        print("Test complete. Stopping devices...")

        for d in self.devices:
            d.stop()
        self.leader.stop()
        print("Done")

    def __restart_children(self):
        devices = [d for d in self.devices if not d.is_router_or_leader()]
        for d in devices:
            d.stop()
        time.sleep(60)
        for d in devices:
            d.start()

    def __restart_random(self, n):
        devices = self.rng.sample(self.devices, n)
        for d in devices:
            d.stop()
        time.sleep(60)
        for d in devices:
            d.start()

    def __loop_until(self, end):
        while time.time() < self.start_time + (end * 60) + 12:
            if time.time() >= self.query_time:
                t = (time.time() - self.start_time) / 60
                print(f"Checkpoint: {t}min")

                self.query_time += QUERY_INTERVAL
                if not DIAG_SERVER:
                    self.leader.send_diag(self.devices)

            time.sleep(10)
            self.leader.process()
            for d in self.devices:
                d.process()


runner = TestRunner()
runner.run()
