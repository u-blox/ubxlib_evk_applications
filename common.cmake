# include the common cmake for the ubxlib, common folder and tasks folder
# Copyright 2024 u-blox
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
 
# Get and build the ubxlib library
# Define where it is located, here we assume it's beneath the current directory
set(UBXLIB_BASE ../ubxlib)
string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)
include(${UBXLIB_BASE}/port/platform/${OS_NAME}/${OS_NAME}.cmake)
target_link_libraries(${APP_NAME} ubxlib ${UBXLIB_REQUIRED_LINK_LIBS})
target_include_directories(${APP_NAME} PUBLIC ${UBXLIB_INC} ${UBXLIB_PUBLIC_INC_PORT})

# ...common utilities
file(REAL_PATH "${CMAKE_SOURCE_DIR}/../common" APP_COMMON_DIR)
file(GLOB APP_COMMONS "${APP_COMMON_DIR}/*.c")

# ...and tasks
file(REAL_PATH "${CMAKE_SOURCE_DIR}/../tasks" APP_TASKS_DIR)
file(GLOB APP_TASKS "${APP_TASKS_DIR}/*.c")

target_sources(${APP_NAME} PRIVATE ${APP_COMMONS} ${APP_TASKS})

file(REAL_PATH "${CMAKE_SOURCE_DIR}/config/" APP_CONFIG)

target_include_directories(${APP_NAME} PRIVATE ${APP_COMMON_DIR} ${APP_TASKS_DIR} ${APP_CONFIG})