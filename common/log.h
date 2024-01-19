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
 * Logging header
 *
 */

#ifndef _LOGGING_H_
#define _LOGGING_H_

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */
// Print to the screen only 
#define printTrace(log, ...)    _writeLog(eTRACE,    false, log, ##__VA_ARGS__)
#define printDebug(log, ...)    _writeLog(eDEBUG,    false, log, ##__VA_ARGS__)
#define printInfo(log, ...)     _writeLog(eINFO,     false, log, ##__VA_ARGS__)
#define printWarn(log, ...)     _writeLog(eWARN,     false, log, ##__VA_ARGS__)
#define printError(log, ...)    _writeLog(eERROR,    false, log, ##__VA_ARGS__)
#define printFatal(log, ...)    _writeLog(eFATAL,    false, log, ##__VA_ARGS__)
#define printAlways(log, ...)   _writeLog(eNOFILTER, false, log, ##__VA_ARGS__)

// Print to the screen and write to the log file
#define writeTrace(log, ...)    _writeLog(eTRACE,    true,  log, ##__VA_ARGS__)
#define writeDebug(log, ...)    _writeLog(eDEBUG,    true,  log, ##__VA_ARGS__)
#define writeInfo(log, ...)     _writeLog(eINFO,     true,  log, ##__VA_ARGS__)
#define writeWarn(log, ...)     _writeLog(eWARN,     true,  log, ##__VA_ARGS__)
#define writeError(log, ...)    _writeLog(eERROR,    true,  log, ##__VA_ARGS__)
#define writeFatal(log, ...)    _writeLog(eFATAL,    true,  log, ##__VA_ARGS__)
#define writeAlways(log, ...)   _writeLog(eNOFILTER, true,  log, ##__VA_ARGS__)

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */

/// @brief Logging level for the printInfo() function
typedef enum {
    eTRACE,
    eDEBUG,
    eINFO,
    eWARN,
    eERROR,
    eFATAL,
    eMAXLOGLEVELS,
    eNOFILTER
} logLevels_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
/// @brief Start logging to the specified file
/// @param pFilename The filename to log to
void startLogging(const char *pFilename);

/// @brief set the logging level of printInfo and writeLog
void setLogLevel(logLevels_t level);

/// @brief Write a log entry to the log file and terminal
/// @param level The level of terminal logging
/// @param writeToFile Set to false to not write to the file
/// @param log The log format to write
/// @param ... The arguments to use in the log entry
void _writeLog(logLevels_t level, bool writeToFile, const char *log, ...);

#endif