#!/bin/bash

function myexit() {
	ret=$1
	shift
	echo "[ERROR] $@" >&2
	exit $ret
}

echo "[INFO] Checking for build tools 'cmake' and 'scons'. Using tools like 'head' and 'sed'."
which cmake &>/dev/null
ret=$?
if ((ret == 0)); then
	tmp=$(cmake --version | head -1)
	CMAKE_MAJOR=$(echo $tmp | sed -E 's/cmake version ([0-9]+)\.([0-9]+)\.([0-9]+)/\1/')
	CMAKE_MINOR=$(echo $tmp | sed -E 's/cmake version ([0-9]+)\.([0-9]+)\.([0-9]+)/\2/')
	((CMAKE_MAJOR < 3)) && myexit 1 "cmake version ($CMAKE_MAJOR) must be at least 3.11"
	((CMAKE_MAJOR == 3)) && ((CMAKE_MINOR < 11)) && myexit 2 "cmake version ($CMAKE_MAJOR) must be at least 3.11"
else
	myexit 3 "Could not find 'cmake'! On ubuntu, you can install it via 'sudo apt install python3-pip; sudo pip install cmake' to obtain the latest version."
fi

which scons &>/dev/null
ret=$?
if ((ret != 0)); then
	myexit 4 "Could not find 'scons'! On ubuntu, you can install it via 'sudo apt install scons' to obtain the latest version."
fi

function checklib() {
	echo -n "[CHECKLIB] $1: "
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

echo -e "[INFO] Trying to find required libraries with 'locate'.\nIf you have a system where this is done in a different way, then please adapt this script."
LOCATE=locate
LIBS_TO_INSTALL=""
checklib libhdf5.so libhdf5 libhdf5-dev
checklib libxerces-c.so libxerces libxerces-c-dev
checklib libelf.so libelf libelf-dev
checklib libconfig++.so libconfig libconfig++-dev

if ((libhdf5 == 0)) || ((libxerces == 0)) || ((libelf == 0)) || ((libconfig == 0)); then
	echo "[QUESTION] Missing libraries${LIBS_TO_INSTALL}. Do you want to install them now? (you need sudo rights to continue)"
	read -n 1 -p "[QUESTION] [Y]es | [n]o" answer
	case "$answer" in
		Y|y|\r|\n|"") sudo apt install ${LIBS_TO_INSTALL}; ;;
		*) ;;
	esac
else
	echo "[INFO] Everything seems to be OK! :-) Trying to build..."
fi

if [[ ! -z "$MBEDTLSPATH" ]]; then
	echo "[INFO] MBEDTLSPATH set."
	[[ -e "$MBEDTLSPATH" ]] || myexit 8 "MBEDTLSPATH is set, but the path it points to does not exist!"
	LIBMEDCRYPTO_PATH=$("$LOCATE" "libmbedcrypto.a")
	if [[ ! -z "${LIBMEDCRYPTO_PATH}" ]]; then
		echo "[INFO] libmbedcrypto.a found at '${LIBMEDCRYPTO_PATH}'"
	else
		echo "[QUESTION] MBEDTLSPATH is set, but required static library libmedcrypto.a was not found. Should I initiate a build and install?"
		echo "[QUESTION] This will create subfolder 'build' in the mbedtls path, then call 'cmake ..' and then 'sudo make -j$(nproc) install'."
		read -n 1 -p "[QUESTION] [Y]es | [n]o" answer
		case "$answer" in
			Y|y|\r|\n|"")
				pushd "$MBEDTLSPATH" &>/dev/null
				mkdir -p build
				pushd build &>/dev/null
				cmake ..
				sudo make -j$(nproc) install
				popd &>/dev/null
				popd &>/dev/null
				;;
			*) ;;
		esac
	fi
else
	echo "[INFO] MBEDTLSPATH not set."
fi

mkdir -p build || myexit 4 "Could not create directory 'build'"
pushd build &>/dev/null || myexit 5 "Could not cd into directory 'build'"
rm -Rf *
cmake .. -DCMAKE_BUILD_TYPE=Release || myexit 6 "Problems in cmake script"
make -j$(nproc) || myexit 7 "Problems running make"
popd
