#!/bin/bash

[[ -e build ]] && [[ -e build/CMakeCache.txt ]] || ./bootstrap.sh

pushd build

# Just to be sure everything is up-to-date
make -j$(nproc)

OUT_DIR="out_$(date +%Y-%m-%d_%H-%M-%S)"
TRACES_FILE="test1.host.traces"
STATS_FILE="test1.host.hmc"
ZSIM_MISC_FILES=(out.cfg zsim.h5 zsim.out zsim-cmp.h5 zsim-ev.h5 heartbeat)

mkdir -p "${OUT_DIR}" || exit 1

# first, run ZSIM to obtain the traces
echo "
##########################
# ZSIM              host #
##########################"
sleep 1
./zsim "../test/test1.host.cfg"

# second, run ramulator
# host: HMC-config.cfg / HMC-config.noCaches.cfg / HMC-config-infinityLLC.cfg
echo "
##########################
# RAMULATOR         host #
##########################"
sleep 1
./ramulator_host --config ../pim-simulators/host/ramulator-host/HMC-Configs/HMC-config.cfg --mode=cpu --stats "${STATS_FILE}" --trace_format=zsim --trace "${TRACES_FILE}" --number-cores=1

mv "${TRACES_FILE}" "${OUT_DIR}/${TRACES_FILE}"
mv "${STATS_FILE}" "${OUT_DIR}/${STATS_FILE}"
for file in "${ZSIM_MISC_FILES[@]}"; do
	mv "${file}" "${OUT_DIR}/test1.host.${file}"
done

popd
