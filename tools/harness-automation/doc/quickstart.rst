Thread Harness Automation Quick Start
=====================================

Setup
-----

#. Install Thread-Test-Harness1.1-Alpha v1.0-Release_13.0
#. Install python 2.7
#. Get OpenThread and switch to the harness automation path::

    git clone https://github.com/openthread/openthread.git
    cd openthread/tools/harness-automation

#. Install python libraries dependencies::

    pip install -r requirements.txt

#. Update settings.

    Just copy the sample and modify::

        cp autothreadharness/settings_sample.py autothreadharness/settings.py

    APC_HOST
        MUST be set to None if no APC PDU available.

    GOLDEN_DEVICES
        can be found on Windows Device Manager. You have to filter out the ports of sniffer device and DUT by yourself.

    OUTPUT_PATH
        MUST be set to a writable directory.

    HARNESS_HOME
        MUST be set to Thread Harness installation directory. e.g. ``C:\GRL\Thread``

Run single case
---------------

::

    # bash
    ./start.sh Router_5_1_1

    # windows command line
    start.bat Router_5_1_1

Run all cases
-------------

::

    # bash
    ./start.sh

    # windows command line
    start.bat

This will record the results in result.json, so that you can continue running cases once broken. You can also get help information with argument -h or --help.

List devices
------------

::

    # bash
    ./start.sh -l

    # windows command line
    start.bat -l

Check single device
-------------------

::

    # bash
    ./start.sh -l COM28

    # windows command line
    start.bat -l COM28

Get Help
---------

::

    # bash
    ./start.sh -h

    # windows command line
    start.bat -h
