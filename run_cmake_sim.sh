#!/bin/bash

cmake -S . -B build_sim/ -DBUILD_FOR_LINUX=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G "Unix Makefiles"
