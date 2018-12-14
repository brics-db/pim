#!/bin/bash
./ramulator_pim --config ../../pim-simulators/pim/ramulator-pim/HMC-Configs/HMC-config-inOrder.cfg --mode=cpu --stats test1.pim.hmc-inOrder --trace ../traces_2018-12-12/test1.pim.traces --pim-cache-lines=0 --pim-org=inOrder --pim-core-count 1 --trace_format zsim
