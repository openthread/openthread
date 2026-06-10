# OpenThread Sniffer Integration with Thread Test Harness

After following the steps below, you will be able to run Test Harness with OpenThread sniffer.

1. require python3 is installed on windows and in the system environment path.
2. require TestHarness is updated on `ReportEngine.pyd`

## Setup

1. install pyspinel (for python3 only)
   ```
   git clone https://github.com/openthread/pyspinel.git
   cd <path-to-pyspinel>
   python setup.py install
   ```
2. Copy "OT_Sniffer.py" to `C:\GRL\Thread1.1\Thread_Harness\Sniffer`.
