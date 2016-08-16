=======================
Harness Automation Tool
=======================

This is a tool to automate testing openthread with GRL Thread-Test-Harness1.1-Alpha v1.0-Release_13.0.

-----------
Quick Start
-----------

Follow the `Quick Start <doc/quickstart.rst>`_

------
Syntax
------

::

 ./start.sh [-h] [--pattern PATTERN] [--delete-blacklist] [--skip SKIP]
                 [--dry-run] [--result-file RESULT_FILE]
                 [--list-file LIST_FILE] [--continue-from CONTINUE_FROM]
                 [--auto-reboot] [--manual-reset] [--list-devices]
                 [NAME [NAME ...]]

-------
Options
-------

If ``-l`` is given, ``NAME`` is the serial port device name. Otherwise ``NAME`` is test case name. This enables running single or multiple test cases directly.

Other options::

    -h, --help
        Show help message and exit.

    --pattern PATTERN, -p PATTERN
        File name pattern, Default to all python files.

    --delete-blacklist, -d
        Clear blacklist on startup. By default, golden devices failed to be connected are kept in a blacklist automatically. Add this option to clear blacklist on startup.

    --skip SKIP, -k SKIP
        Type of test case status to skip. ``e`` for error, ``f`` for fail, ``p`` for pass. Default to "efp". If test case names are given by ``NAME``, this option will not work.

    --dry-run, -n
        Just show what test case will be run.

    --result-file RESULT_FILE, -o RESULT_FILE
        File to store and read current status.

    --list-file LIST_FILE, -i LIST_FILE
        File to list cases names to test.

    --continue-from CONTINUE_FROM, -c CONTINUE_FROM
        First case to test.

    --auto-reboot, -a
        Restart system when harness cannot recover. This is a walk around to some issues, such as golden devices may not work after long time running and have to restart system.

    --manual-reset, -m
        Reset devices manually, in case no PDU available so that reset golden devices have to be done manually.

    --list-devices, -l
        List devices, including their version information.
