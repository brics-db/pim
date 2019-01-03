#!/bin/bash

[[ -e build ]] && [[ -e build/CMakeCache.txt ]] || ./bootstrap.sh

pushd build

OUT_DIR="out_$(date +%Y-%m-%d_%H-%M-%S)"
TRACES_FILE="test1.pim.traces"
STATS_FILE="test1.pim.hmc"
ZSIM_MISC_FILES=(out.cfg zsim.h5 zsim.out zsim-cmp.h5 zsim-ev.h5 heartbeat)

mkdir -p "${OUT_DIR}" || exit 1

# first, run ZSIM to obtain the traces
./zsim "../test/test1.pim.cfg"

# second, run ramulator
# pim: HMC-config.cfg / HMC-config-OoOrder.cfg / HMC-config-OoOrder-512.cfg / HMC-config-inOrder.cfg
./ramulator_pim --config ../pim-simulators/pim/ramulator-pim/HMC-Configs/HMC-config-inOrder.cfg --mode=cpu --stats "${STATS_FILE}" --trace_format=zsim --trace "${TRACES_FILE}" --pim-core-count=1 --pim-cache-lines=0 --pim-org=inOrder

mv "${TRACES_FILE}" "${OUT_DIR}/${TRACES_FILE}"
mv "${STATS_FILE}" "${OUT_DIR}/${STATS_FILE}"
for file in "${ZSIM_MISC_FILES[@]}"; do
	mv "${file}" "${OUT_DIR}/test1.pim.${file}"
done

popd
