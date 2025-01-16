#!/bin/sh

cmake -S . -B build -DPICO_BOARD=pico_w -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DWIFI_SSID="\"$WIFI_SSID\"" -DWIFI_PASSWORD="\"$WIFI_PASSWORD\"" -G "Unix Makefiles"

