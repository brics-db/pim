#!/bin/bash
[[ -e build ]] && [[ -e CMakeCache.txt ]] || ./bootstrap.sh

TRACES_DIR="traces_$(date +%Y-%m-%d)"
mkdir "${TRACES_DIR}" || exit 1

# first, run ZSIM to obtain the traces
build/zsim test/test1.host.cfg
build/zsim test/test1.pim.cfg

# then, run
build/ramulator_host --config pim-simulators/host/ramulator-host/HMC-Configs/HMC-config.cfg --mode=cpu --stats "${TRACES_DIR}/test1.host.hmc" --trace_format=zsim --trace traces_2018-12-12/test1.host.traces --number-cores=1
build/ramulator_host --config pim-simulators/host/ramulator-host/HMC-Configs/HMC-config.cfg --mode=cpu --stats "${TRACES_DIR}/test1.host.hmc" --trace_format=zsim --trace traces_2018-12-12/test1.host.traces --number-cores=1
