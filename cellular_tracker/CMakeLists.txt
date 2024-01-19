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
cmake_minimum_required(VERSION 3.19)

set(APP_NAME cellular_tracker)
project(${APP_NAME})

# This application file(s)
add_executable(${APP_NAME} src/main.c)

include(../common.cmake)