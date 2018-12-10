#!/bin/bash

function myexit() {
	ret=$1
	shift
	echo "$@" >&2
	exit $ret
}

mkdir -p build || exit 1 "Could not create directory 'build'"
pushd build &>/dev/null || exit 2 "Could not cd into directory 'build'"
rm -Rf *
cmake .. -DCMAKE_BUILD_TYPE=Release || exit 3 "Problems in cmake script"
make -j$(nproc) || exit 4 "Problems running make"
popd
