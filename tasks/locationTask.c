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
 * Location Task to publish the position of the XPLR device to the cloud
 *
 */

#include <time.h>
#include "common.h"
#include "taskControl.h"
#include "locationTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define LOCATION_TASK_STACK_SIZE    (3 * 1024)
#define LOCATION_TASK_PRIORITY      5

#define LOCATION_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define LOCATION_QUEUE_PRIORITY 5
#define LOCATION_QUEUE_SIZE     5

#define JSON_STRING_LENGTH      300

#define TEN_MILLIONTH           10000000

/* ----------------------------------------------------------------
 * EXTERNAL VARIABLES
 * -------------------------------------------------------------- */
extern int32_t gnssModuleType;

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uNetworkCfgGnss_t gNetworkGNSSCfg = {
    .type = U_NETWORK_TYPE_GNSS,
    .moduleType = U_GNSS_MODULE_TYPE_M8,
    .devicePinPwr = -1,
    .devicePinDataReady = -1
};

static uDeviceCfg_t gnssDeviceCfg;
static uDeviceHandle_t *pGnssHandle;

static bool stopLocation = false;

static char topicName[MAX_TOPIC_NAME_SIZE];

/// callback commands for incoming MQTT control messages
static callbackCommand_t callbacks[] = {
    {"LOCATION_NOW", queueLocationNow},
    {"START_TASK", startLocationTaskLoop},
    {"STOP_TASK", stopLocationTaskLoop}
};

/// buffer for the location MQTT JSON message
static char jsonBuffer[JSON_STRING_LENGTH];

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief check if the application is exiting, or task stopping
static bool isNotExiting(void)
{
    return !gExitApp && !exitTask && !stopLocation;
}

static bool keepGoing(void *pParam)
{
    bool keepGoing = isNotExiting();
    if (keepGoing) {
        printDebug("Waiting for GNSS location...");
    } else {
        printDebug("GNSS location cancelled");
    }

    return keepGoing;
}

static char fractionConvert(int32_t x1e7,
                            int32_t divider,
                            int32_t *pWhole,
                            int32_t *pFraction)
{
    char prefix = ' ';

    // Deal with the sign
    if (x1e7 < 0) {
        x1e7 = -x1e7;
        prefix = '-';
    }
    *pWhole = x1e7 / divider;
    *pFraction = x1e7 % divider;

    return prefix;
}

static void publishLocation(uLocation_t location)
{
    int32_t whole = 0;
    int32_t fraction = 0;

    gAppStatus = LOCATION_MEAS;

    char timestamp[TIMESTAMP_MAX_LENGTH_BYTES];
    getTimeStamp(timestamp);

    char format[] = "{"                     \
            "\"Timestamp\":\"%s\", "        \
            "\"Location\":{"                \
                "\"Altitude\":%d, "         \
                "\"Latitude\":%c%d.%07d, "  \
                "\"Longitude\":%c%d.%07d, " \
                "\"Accuracy\":%d, "         \
                "\"Speed\":%d, "            \
                "\"utcTime\":\"%lld\"}"     \
        "}";

    struct tm *t = gmtime((time_t *)&location.timeUtc);

    int32_t latWhole, latFraction, longWhole, longFraction;
    char latPrefix  = fractionConvert(location.latitudeX1e7,  TEN_MILLIONTH, &latWhole,  &latFraction);
    char longPrefix = fractionConvert(location.longitudeX1e7,  TEN_MILLIONTH, &longWhole, &longFraction);

    snprintf(jsonBuffer, JSON_STRING_LENGTH, format, timestamp,
            location.altitudeMillimetres,
            latPrefix, latWhole, latFraction,
            longPrefix, longWhole, longFraction,
            location.radiusMillimetres,
            location.speedMillimetresPerSecond,
            location.timeUtc);

    writeAlways(jsonBuffer);
    publishMQTTMessage(topicName, jsonBuffer, U_MQTT_QOS_AT_MOST_ONCE, true);
}

static void getLocation(void *pParams)
{
    if (uPortMutexTryLock(TASK_MUTEX, 0) == 0) {
        uLocation_t location;   
        printDebug("Requesting location information...");
        int32_t errorCode = uLocationGet(*pGnssHandle, U_LOCATION_TYPE_GNSS,
                                            NULL, NULL, &location, keepGoing);
        if (errorCode == 0) {
            printDebug("Got location information [%d, %d], publishing", location.latitudeX1e7, location.longitudeX1e7);
            publishLocation(location);
        } else {
            if (errorCode == U_ERROR_COMMON_TIMEOUT)
                writeDebug("Timed out getting GNSS location");
            else
                writeError("Failed to get GNSS location: %d", errorCode);
        }

        // reset the stop location indicator
        stopLocation = false;

        uPortMutexUnlock(TASK_MUTEX);
    } else {
        printDebug("getLocation(): Already trying to get location.");
    }
}

static void startGetLocation(void)
{
    RUN_FUNC(getLocation, LOCATION_TASK_STACK_SIZE, LOCATION_TASK_PRIORITY);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    locationMsg_t *qMsg = (locationMsg_t *) pParam;

    switch(qMsg->msgType) {
        case GET_LOCATION_NOW:
            startGetLocation();
            break;

        case STOP_LOCATION_ACQUISITION:
            stopLocation = true;
            break;

        case SHUTDOWN_LOCATION_TASK:
            stopLocationTaskLoop(NULL);
            break;

        default:
            writeInfo("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

// Signal Quality task loop for reading the RSRP and RSRQ values
// and sending these values to the MQTT topic
static void taskLoop(void *pParameters)
{
    while(isNotExiting()) {
        getLocation(NULL);
        dwellTask(taskConfig, isNotExiting);
    }

    FINALIZE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(locationMsg_t),
                    LOCATION_QUEUE_STACK_SIZE,
                    LOCATION_QUEUE_PRIORITY,
                    LOCATION_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeFatal("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

static int32_t startGNSS(void)
{
    int32_t errorCode;

    // Simply copy the gCellDeviceHandle to the pGnssHandle variable
    // as this application is for the EVK with Combo modules or GNSS I2C Adapter boards
    pGnssHandle = &gCellDeviceHandle;

    // set the correct GNSS module type being used
    gNetworkGNSSCfg.moduleType = gnssModuleType;

    errorCode = uNetworkInterfaceUp(*pGnssHandle, U_NETWORK_TYPE_GNSS, &gNetworkGNSSCfg);

    if (errorCode != 0) {
        writeFatal("Failed to bring up the GNSS device: %d", errorCode);
        return errorCode;
    }

    return U_ERROR_COMMON_SUCCESS;
}

static int32_t initMutex()
{
    INIT_MUTEX;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Queue the getLocation operation
/// @param params The parameters for this command
/// @return returns the errorCode of sending the message on the eventQueue
int32_t queueLocationNow(commandParamsList_t *params)
{
    locationMsg_t qMsg;
    qMsg.msgType = GET_LOCATION_NOW;

    return sendAppTaskMessage(TASK_ID, &qMsg, sizeof(locationMsg_t));
}

/// @brief Initialises the Signal Quality task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initLocationTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    CREATE_TOPIC_NAME;

    writeInfo("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);

    result = startGNSS();
    if (result < 0) {
        writeFatal("Failed to start the GNSS system");
        return result;
    }

    char tp[MAX_TOPIC_NAME_SIZE];
    snprintf(tp, MAX_TOPIC_NAME_SIZE, "%sControl", TASK_NAME);
    subscribeToTopicAsync(tp, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successful, a negative number otherwise
int32_t startLocationTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    if (params != NULL)
        taskConfig->taskLoopDwellTime = getParamValue(params, 1, 5, 60, 30);

    START_TASK_LOOP(LOCATION_TASK_STACK_SIZE, LOCATION_TASK_PRIORITY);
}

int32_t stopLocationTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}

int32_t finalizeLocationTask(void)
{
    return U_ERROR_COMMON_SUCCESS;
}