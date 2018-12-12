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
    PROJECT(BRICS_DB_PIM VERSION 0.9 LANGUAGES CXX)
ELSE()
    PROJECT(BRICS_DB_PIM LANGUAGES CXX)
ENDIF()

set(CMAKE_DISABLE_IN_SOURCE_BUILD OFF)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

set(CMAKE_COLOR_MAKEFILE ON) 
# And optionally
#set(CMAKE_VERBOSE_MAKEFILE ON)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")
INCLUDE(cotire)

ADD_SUBDIRECTORY(pim-simulators)

ADD_EXECUTABLE(test1 "${PROJECT_SOURCE_DIR}/test/test1.cpp")
TARGET_INCLUDE_DIRECTORIES(test1 BEFORE PUBLIC "${PROJECT_SOURCE_DIR}/pim-simulators/zsim-ramulator/misc/hooks")
TARGET_COMPILE_OPTIONS(test1 PUBLIC -Wall -Wextra -pedantic -Weffc++ -fexceptions -fnon-call-exceptions -march=native)
SET_PROPERTY(TARGET test1 PROPERTY CXX_STANDARD 17)
SET_PROPERTY(TARGET test1 PROPERTY CXX_STANDARD_REQUIRED ON)

SET(ZSIM_PATH "${CMAKE_SOURCE_DIR}/pim-simulators/zsim-ramulator/build/opt/")
IF(EXISTS "${ZSIM_PATH}")
	SET(ZSIM_EXECUTABLE ${CMAKE_BINARY_DIR}/zsim)
	SET(ZSIM_LIBRARY ${CMAKE_BINARY_DIR}/libzsim.so)
	ADD_CUSTOM_COMMAND(OUTPUT "${ZSIM_EXECUTABLE}" COMMAND ${CMAKE_COMMAND} -E copy "${ZSIM_PATH}/zsim" "${CMAKE_BINARY_DIR}")
	ADD_CUSTOM_COMMAND(OUTPUT "${ZSIM_LIBRARY}" COMMAND ${CMAKE_COMMAND} -E copy "${ZSIM_PATH}/libzsim.so" "${CMAKE_BINARY_DIR}")
	ADD_CUSTOM_TARGET(copy_zsim ALL DEPENDS "${ZSIM_EXECUTABLE}" "${ZSIM_LIBRARY}")
ELSE()
	MESSAGE(FATAL_ERROR "Required zsim executable path (${ZSIM_PATH}) not found!")
ENDIF()


