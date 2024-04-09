/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * Configuration for the Cellular Tracking Application
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* ----------------------------------------------------------------
 * Application Version number - this includes the common/tasks too
 * -------------------------------------------------------------- */
#define APP_NAME    "Cellular Tracker"
#define APP_VERSION "v1.2"

/* ----------------------------------------------------------------
 * Application Build Target - select what target system this is for
 * -------------------------------------------------------------- */
//#define BUILD_TARGET_RASPBERRY_PI       // Raspberry PI
#define BUILD_TARGET_WINDOWS              // Windows

/* ----------------------------------------------------------------
 * APPLICATION DEBUG LEVEL SETTING 
 *                          This can be changed remotely using
 *                          "SET_LOG_LEVEL" command via the
 *                          APP_CONTROL MQTT topic
 * -------------------------------------------------------------- */
//#define LOGGING_LEVEL eINFO            // taken from logLevels_t
#define LOGGING_LEVEL eDEBUG            // taken from logLevels_t

#endif
