#!/bin/sh

cmake -S . -B build -DPICO_BOARD=pico_w -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G "Unix Makefiles"

