=======================
Harness Automation Tool
=======================

This is a tool to automate testing openthread with GRL Thread-Test-Harness1.1-Alpha v1.0-Release_40.0.

-----------
Get Started
-----------

To get started, follow the `Automation Setup Guide <https://openthread.io/certification/automation_setup>`.

------
Syntax
------

::

 ./start.sh [-h] [--pattern PATTERN] [--delete-blacklist] [--skip SKIP]
                 [--dry-run] [--result-file RESULT_FILE]
                 [--list-file LIST_FILE] [--continue-from CONTINUE_FROM]
                 [--auto-reboot] [--manual-reset] [--list-devices]
                 [NAME [NAME ...]]

If ``-l`` is given, ``NAME`` is the serial port device name. Otherwise ``NAME`` is test case name. This enables running single or multiple test cases directly.

Use ``./start.sh --help`` for a full list of options.