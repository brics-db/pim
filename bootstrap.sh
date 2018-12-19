#!/bin/bash

function myexit() {
	ret=$1
	shift
	echo "$@" >&2
	exit $ret
}

echo "Checking for build tools 'cmake' and 'scons'. Using tools like 'head' and 'sed'."
which cmake &>/dev/null
ret=$?
if ((ret == 0)); then
	tmp=$(cmake --version | head -1)
	CMAKE_MAJOR=$(echo $tmp | sed -E 's/cmake version ([0-9]+)\.([0-9]+)\.([0-9]+)/\1/')
	CMAKE_MINOR=$(echo $tmp | sed -E 's/cmake version ([0-9]+)\.([0-9]+)\.([0-9]+)/\2/')
	if ((CMAKE_MAJOR < 3)); then echo "cmake version ($CMAKE_MAJOR) must be at least 3.11"; exit 1; fi
	if ((CMAKE_MAJOR == 3)) && ((CMAKE_MINOR < 11)); then myexit 1 "cmake version ($CMAKE_MAJOR) must be at least 3.11"; fi
else
	myexit 2 "Could not find 'cmake'! On ubuntu, you can install it via 'sudo apt install python3-pip; sudo pip install cmake' to obtain the latest version."
fi

which scons &>/dev/null
ret=$?
if ((ret != 0)); then
	echo "Could not find 'scons'! On ubuntu, you can install it via 'sudo apt install scons' to obtain the latest version."
fi

function checklib() {
	echo -n "$1: "
	"$LOCATE" $1 &>/dev/null
	ret=$?
	if ((ret == 0)); then
		echo "found."
		eval "$2=1"
		return 1
	else
		echo "not found!"
		eval "$2=0"
		LIBS_TO_INSTALL="${LIBS_TO_INSTALL} $3"
		return 0
	fi
}

echo -e "Trying to find required libraries with 'locate'.\nIf you have a system where this is done in a different way, then please adapt this script."
LOCATE=locate
LIBS_TO_INSTALL=""
checklib libhdf5.so libhdf5 libhdf5-dev
checklib libxerces-c.so libxerces libxerces-c-dev
checklib libelf.so libelf libelf-dev
checklib libconfig++.so libconfig libconfig++-dev

if ((libhdf5 == 0)) || ((libxerces == 0)) || ((libelf == 0)) || ((libconfig == 0)); then
	echo "Missing libraries${LIBS_TO_INSTALL}. Do you want to install them now? (you need sudo rights to continue)"
	read -pn1 "[Y]es | [n]o" answer
	case "$answer" in
		Y|y|\r|\n) sudo apt install ${LIBS_TO_INSTALL}; ;;
		*) ;;
	esac
else
	echo "Everything seems to be OK! :-) Trying to build..."
fi

mkdir -p build || exit 1 "Could not create directory 'build'"
pushd build &>/dev/null || exit 2 "Could not cd into directory 'build'"
rm -Rf *
cmake .. -DCMAKE_BUILD_TYPE=Release || exit 3 "Problems in cmake script"
make -j$(nproc) || exit 4 "Problems running make"
popd
