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
 * Logging functions
 *
 */

#include <stdarg.h>
#include <time.h>

#include "common.h"
#include "log.h"

/* Currently we don't support writing to the log FILE */
//#include "filesystem.h"

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */

/// DO NOT PUT printInfo() INSIDE this MUTEX LOCK!!!
#define MUTEX_LOCK if (pLogMutex != NULL) uPortMutexLock(pLogMutex); {
#define MUTEX_UNLOCK } if (pLogMutex != NULL) uPortMutexUnlock(pLogMutex);

#define LOG_BUFFER_SIZE 2048

#define FILE_READ_BUFFER 512

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char buff1[LOG_BUFFER_SIZE+1];
static char buff2[LOG_BUFFER_SIZE+1];
static char timeStamp[TIMESTAMP_MAX_LENGTH_BYTES+1];

static FILE logFile;

static bool logFileOpen = false;

static uPortMutexHandle_t pLogMutex = NULL;
static uPortTimerHandle_t pFlushTimerHandle = NULL;

static logLevels_t gLogLevel = eINFO;

static bool flushLogFileCache = false;

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */
/// The unix network time, which is retrieved after first registration
int64_t unixNetworkTime = 0;

// The tick time of the OS when the unix network time was acquired.
int32_t bootTicksTime = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static bool printHeader(logLevels_t level, bool writeToFile)
{
    const char *header = NULL;
    switch(level) {
        case eWARN:
            header = "\n*** WARNING ************************************************\n";
            break;

        case eERROR:
            header = "\n************************************************************\n" \
                     "*** ERROR **************************************************\n";
            break;

        case eFATAL:
            header = "\n############################################################\n" \
                     "#### FATAL ** FATAL ** FATAL ** FATAL ** FATAL ** FATAL ####\n" \
                     "############################################################\n";
            break;

        default:
            // no header
            break;
    }

    if (header == NULL)
        return false;

    printf("%s", header);

    /* Currently we don't support writing to the log FILE
    if (logFileOpen && writeToFile)
        fs_write(&logFile, header, strlen(header));
    */

    return true;
}

static void flushTimerCallback(void *callbackHandle, void *param)
{
    flushLogFileCache = true;
}
static bool openLogFile(const char *pFilename)
{
    return -1;

    /* Currently we don't support writing to the log FILE
    fs_file_t_init(&logFile);
    const char *path = extFsPath(pFilename);
    int result = fs_open(&logFile, path, FS_O_APPEND | FS_O_CREATE | FS_O_RDWR);

    if (result == 0) {
        printInfo("File logging enabled");
        logFileOpen = true;
    } else {
        printError("Failed to open log file: %d. Logging to the log file will not be available.", result);
        logFileOpen = false;
    }

    return logFileOpen;
    */
}

static int32_t createLogFileMutex(void)
{
    int32_t errorCode = uPortMutexCreate(&pLogMutex);
    if (errorCode < 0)
        printError("Failed to create the log mutex: %d\n Logging to the log file will not be available.", errorCode);

    return errorCode;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
void setLogLevel(logLevels_t logLevel)
{
    printInfo("Setting log level to %d", logLevel);
    gLogLevel = logLevel;
}

void _writeInfoToFile(bool header)
{
    /** Currently we do not support writing to the log FILE 
    int result = fs_write(&logFile, buff2, strlen(buff2));
    if (result < 0) {
        printf("Failed to write to log file: %d", result);
    }

    if (header) {
        result = fs_write(&logFile, "\n", 1);
        if (result < 0) {
            printf("Failed to write to log file: %d", result);
        }
    }

    if (flushLogFileCache) {
        flushLogFileCache = false;
        printf("Flushing log file...");
        int result = fs_sync(&logFile);
        if (result != 0)
            printf("FAILED: %d", result);
        else
            printf("Done.\n");
    }
    */
}

/// @brief Writes a log message to the terminal and the log file
/// @param log The log, which can contain string formatting
/// @param  ... The variables for the string format
void _writeLog(logLevels_t level, bool writeToFile, const char *log, ...)
{
    if (level < gLogLevel)
        return;

    MUTEX_LOCK

        // Construct the application's arguments into a log string
        va_list arg_list;
        va_start(arg_list, log);
        vsnprintf(buff1, LOG_BUFFER_SIZE, log, arg_list);
        va_end(arg_list);

        // DO NOT PUT PRINTLOG OR WRITELOG MARCOS INSIDE
        // THIS MUTEX LOCK - *ONLY* USE PRINTF() !!!!!!

        // ...add the timestamp to the start
        getTimeStamp(timeStamp);
        snprintf(buff2, LOG_BUFFER_SIZE, "%s: %s\n", timeStamp, buff1);

        bool header = printHeader(level, writeToFile);
        printf("%s", buff2);
        if (header)
            printf("\n");

        if (logFileOpen && writeToFile) {
            _writeInfoToFile(header);
        }

        // DO NOT PUT PRINTLOG OR WRITELOG MARCOS INSIDE
        // THIS MUTEX LOCK - *ONLY* USE PRINTF() !!!!!!
    
    MUTEX_UNLOCK;
}

/// @brief Close the log file
void closeLogFile(bool displayWarning)
{
    if (!logFileOpen)
        return;

    if (displayWarning)
        printf("\nClosing log file... PLEASE WAIT!!!\n");
    
    MUTEX_LOCK

        logFileOpen = false;

        /* Currently we don't support writing to the log FILE */
        // fs_close(&logFile);

        if (pFlushTimerHandle != NULL)
            uPortTimerStop(pFlushTimerHandle);
    
    MUTEX_UNLOCK
    
    if (displayWarning)
        printf("Log file is now closed.\n");
}

void displayLogFile(void)
{
    char buffer[FILE_READ_BUFFER];
    int count;

    if (!logFileOpen) {
        printf("Opening log file failed, cannot display log.");
        return;
    }

    printf("\n********************************************************\n"
               "*** LOG START ******************************************\n"
               "********************************************************\n");

    /* Currently we don't support writing to the log FILE
    while((count = fs_read(&logFile, buffer, FILE_READ_BUFFER)) > 0)
        printf("%.*s", count, buffer);
    */

    printf("\n********************************************************\n"
               "*** LOG END ********************************************\n"
               "********************************************************\n");
}

void deleteFile(const char *pFilename)
{
    /* Currently we don't support writing to the log FILE

    const char *path = fsPath(pFilename);
    if (fs_unlink(path) == 0)
        printInfo("Deleted file: %s", pFilename);
    else
        printInfo("Failed to delete file: %s", pFilename);

    */
}

void startLogging(const char *pFilename) {
    if (createLogFileMutex() < 0)
        return;

    if (openLogFile(pFilename) == 0) {
        // success!
    } else {
        printInfo("Only logging to terminal is enabled, not file logging.");
    }
}