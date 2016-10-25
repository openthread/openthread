#!/bin/sh
cd $(dirname $0)
python -u -m autothreadharness.runner $@
