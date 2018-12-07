# Copyright (c) 2018 Till Kolditz
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.8 FATAL_ERROR)

IF(POLICY CMP0048)
    CMAKE_POLICY(SET CMP0048 NEW)
    PROJECT(AHEAD VERSION 0.9 LANGUAGES CXX)
ELSE()
    PROJECT(AHEAD LANGUAGES CXX)
ENDIF()

set(CMAKE_DISABLE_IN_SOURCE_BUILD OFF)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

set(CMAKE_COLOR_MAKEFILE ON) 
# And optionally
#set(CMAKE_VERBOSE_MAKEFILE ON)

#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Weffc++ -fexceptions -fnon-call-exceptions")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -rdynamic")

#SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
# see
# http://stackoverflow.com/questions/11437692/how-to-add-a-custom-build-type-to-cmake-targetting-make
# for adding custom build types
#SET(CMAKE_CXX_FLAGS_PROFILEGENERATE "${CMAKE_CXX_FLAGS_PROFILEGENERATE} -O0 -DDEBUG -rdynamic -g -pg -fprofile-generate")
#SET(CMAKE_CXX_FLAGS_PROFILEUSE "${CMAKE_CXX_FLAGS_PROFILEUSE} -O3 -DNDEBUG -fprofile-use")

SET(NPROC "1")
EXECUTE_PROCESS(COMMAND nproc RESULT_VARIABLE NPROC_RETURNCODE OUTPUT_VARIABLE NPROC_OUT ERROR_VARIABLE NPROC_ERR OUTPUT_STRIP_TRAILING_WHITESPACE)
IF(NOT NPROC_RETURNCODE EQUAL 0)
	MESSAGE(WARNING "Could not use nproc to determine number of cores! Not using parallel compilation!")
ELSE(NOT NPROC_RETURNCODE EQUAL 0)
	SET(NPROC "${NPROC_OUT}")
ENDIF(NOT NPROC_RETURNCODE EQUAL 0)
EXECUTE_PROCESS(COMMAND which scons RESULT_VARIABLE SCONS_RETURNCODE)
IF(NOT SCONS_RETURNCODE EQUAL 0)
	MESSAGE(FATAL_ERROR "scons is not installed! Install using: sudo apt install scons")
ENDIF(NOT SCONS_RETURNCODE EQUAL 0)
EXECUTE_PROCESS(COMMAND git submodule update --init --recursive WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
EXECUTE_PROCESS(COMMAND make -j "${NPROC}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/pim-simulators/pim/DRAMPower")
EXECUTE_PROCESS(COMMAND scons -j "${NPROC}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/pim-simulators/pim/DRAMPower")
EXECUTE_PROCESS(COMMAND make -j "${NPROC}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/pim-simulators/pim/ramulator-pim")
EXECUTE_PROCESS(COMMAND make -j "${NPROC}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/pim-simulators/host/ramulator-host")
EXECUTE_PROCESS(COMMAND scons -j "${NPROC}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/pim-simulators/zsim-ramulator")

ADD_EXECUTABLE(test1 "${CMAKE_SOURCE_DIR}/test/test1.cpp")
TARGET_INCLUDE_DIRECTORIES(test1 BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/pim-simulators/zsim-ramulator/misc/hooks")
