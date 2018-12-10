#!/bin/bash

echo "We assume a default build directory in the root 'build'"

echo "****************"
echo "ZSIMulating host"
echo "****************"
../build/zsim ./test1.host.cfg

echo "****************"
echo " ZSIMulating PIM"
echo "****************"
../build/zsim ./test1.pim.cfg

echo "****************"
echo " RAMulating host"
echo "****************"
../pim-simulators/host/ramulator-host


echo "****************"
echo "  RAMulating PIM"
echo "****************"
