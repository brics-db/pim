#!/bin/bash

[[ -e build ]] && [[ -e build/CMakeCache.txt ]] || ./bootstrap.sh

pushd build

# Just to be sure everything is up-to-date
make -j$(nproc)

OUT_DIR="out_$(date +%Y-%m-%d_%H-%M-%S)"
export OUT_DIR
TRACES_FILE_HOST="test1.host.traces"
STATS_FILE_HOST="test1.host.hmc"
TRACES_FILE_PIM="test1.pim.traces"
STATS_FILE_PIM="test1.pim.hmc"
ZSIM_MISC_FILES=(out.cfg zsim.h5 zsim.out zsim-cmp.h5 zsim-ev.h5 heartbeat)

mkdir -p "${OUT_DIR}" || exit 1

# first, run host
echo "
##########################
# ZSIM              host #
##########################"
./zsim "../test/test1.host.cfg"
# host: HMC-config.cfg / HMC-config.noCaches.cfg / HMC-config-infinityLLC.cfg
echo "
##########################
# RAMULATOR         host #
##########################"
./ramulator_host --config ../pim-simulators/host/ramulator-host/HMC-Configs/HMC-config.cfg --mode=cpu --stats "${STATS_FILE_HOST}" --trace_format=zsim --trace "${TRACES_FILE_HOST}" --number-cores=1

mv "${TRACES_FILE_HOST}" "${OUT_DIR}/${TRACES_FILE_HOST}"
mv "${STATS_FILE_HOST}" "${OUT_DIR}/${STATS_FILE_HOST}"
for file in "${ZSIM_MISC_FILES[@]}"; do
	mv "${file}" "${OUT_DIR}/test1.host.${file}"
done

# second, run pim
echo "
##########################
# ZSIM               pim #
##########################"
sleep 1
./zsim "../test/test1.pim.cfg"
# pim: HMC-config.cfg / HMC-config-OoOrder.cfg / HMC-config-OoOrder-512.cfg / HMC-config-inOrder.cfg
echo "
##########################
# RAMULATOR          pim #
##########################"
sleep 1
./ramulator_pim --config ../pim-simulators/pim/ramulator-pim/HMC-Configs/HMC-config-inOrder.cfg --mode=cpu --stats "${STATS_FILE_PIM}" --trace_format=zsim --trace "${TRACES_FILE_PIM}" --pim-core-count=1 --pim-cache-lines=0 --pim-org=inOrder

mv "${TRACES_FILE_PIM}" "${OUT_DIR}/${TRACES_FILE_PIM}"
mv "${STATS_FILE_PIM}" "${OUT_DIR}/${STATS_FILE_PIM}"
for file in "${ZSIM_MISC_FILES[@]}"; do
	mv "${file}" "${OUT_DIR}/test1.pim.${file}"
done

popd
