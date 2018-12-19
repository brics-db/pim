#!/bin/bash
[[ -e build ]] && [[ -e CMakeCache.txt ]] || ./bootstrap.sh
mkdir "traces_$(date +%Y-%m-%d)" || exit 1
build/ramulator_host --config pim-simulators/host/ramulator-host/HMC-Configs/HMC-config.cfg --mode=cpu --stats test1.host.hmc --trace_format=zsim --trace traces_2018-12-12/test1.host.traces --number-cores=1
