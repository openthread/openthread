#!/bin/bash

# Copyright 2019 NXP
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Script gets as first command line argument the directory
# containing the ELF files which will be signed using
# $SIGNING_TOOL. After the signing process, the binary
# is extracted from the ELF file and placed in an equivalent
# filename with the .bin extension.


function is_linux_package_installed()
{
	dpkg -s ${1} &> /dev/null

	if [ $? -ne 0 ]; then
        echo "Linux package ** ${1} ** is not installed! Please install it then recompile."
        echo "Installation command (Debian-based Linux system): sudo apt-get install ${1}"
		exit 1
	fi
}

function is_python_package_installed()
{
	pip list --format=columns | grep -F ${1} &> /dev/null

	if [ $? -ne 0 ]; then
        echo "Python package ** ${1} ** is not installed! Please install it then recompile."
        echo "Installation command: pip install ${1}"
		exit 1
	fi
}

is_linux_package_installed "python"
is_linux_package_installed "python-pip"
is_python_package_installed "pycrypto"
is_python_package_installed "pycryptodome"

CURR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SIGNING_TOOL=$CURR_DIR/dk6_image_tool.py
MIME_PATTERN="application/x-executable"

if [ "$#" -eq 1 ]; then
	DIR_WITH_ELFS=$1

	for FILENAME in $DIR_WITH_ELFS/*; do
		MIME_SET="$(file -ib $FILENAME)"

		if [[ $MIME_SET == *"$MIME_PATTERN"* ]]; then
			python $SIGNING_TOOL $FILENAME
			arm-none-eabi-objcopy -O binary $FILENAME $FILENAME.bin
		fi
	done
fi
