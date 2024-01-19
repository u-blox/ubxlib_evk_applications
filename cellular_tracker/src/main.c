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
 * Cellular tracking application based on the XPLR-IoT-1 device
 * Connects to an MQTT broker and publishes:
 *      o cellular rsrp/rsrq measurements
 *      o cellular CellID
 *      o GNSS Location
 *
 */

#include "common.h"
#include "appInit.h"
#include "taskControl.h"
#include "cellInit.h"

#include "mqttTask.h"
#include "signalQualityTask.h"
#include "locationTask.h"
#include "cellScanTask.h"

#include <signal.h>

//#include "u_mutex_debug.h"

// Application name and version number is in the config.h file

// Command line argument <ttyUART> which represents the TTY device for the cellular module
#define MAX_TTY_UART_NAME 20
char ttyUART[MAX_TTY_UART_NAME+1];

// OR - for windows we need the comport number
int32_t comPortNumber = 0;

// Command line argument <cellModuleType> which represents the u_cell_module_type.h value
int32_t cellModuleType = -1;

// Command line argument <gnssModuleType> which represents the u_cell_module_type.h value
int32_t gnssModuleType = -1;

// Command line argument [config] which represents the configuration file to use
#define CONFIGURATION_FILENAME "config.txt"
#define MAX_CONFIG_FILENAME 200
char configFileName[MAX_CONFIG_FILENAME+1];

#define NOT_ENOUGH_ARGUMENTS -1
#define TTY_UART_NAME_TOO_BIG -2
#define UNSUPPORTED_CELL_MODULE -3
#define UNSUPPORTED_GNSS_MODULE -4
#define CONFIG_FILENAME_TOO_BIG -5

/* ----------------------------------------------------------------
 * Remote control callbacks for the main application
 * Add your application topic message callbacks here
 * -------------------------------------------------------------- */
#define APP_CONTROL_TOPIC "AppControl"
static callbackCommand_t callbacks[] = {
    {"SET_DWELL_TIME", setAppDwellTime},
    {"SET_LOG_LEVEL", setAppLogLevel}
};

/// @brief The application function(s) which are run every appDwellTime
/// @return A flag to indicate the application should continue (true)
bool appFunction(void)
{
    static int32_t networkBackUpCounter = 0;
    static bool prevNetworkAvailable = false;

    prevNetworkAvailable = IS_NETWORK_AVAILABLE;
    queueMeasureNow(NULL);

    if (IS_NETWORK_AVAILABLE == true && (
        networkBackUpCounter == 0 || prevNetworkAvailable == false)) {
        printDebug("Network is now back up");
        networkBackUpCounter++;
        publishCellularModuleInfo(networkBackUpCounter);
    }

    queueLocationNow(NULL);

    return true;
}

void buttonTwo(void)
{
    queueNetworkScan(NULL);
}

bool networkIsUp(void)
{
    return gIsNetworkUp;
}

bool mqttConnectionIsUp(void)
{
    return gIsMQTTConnected;
}

/// @brief Interrupt for SIGNAL INTERRUPT signal
/// @param value    not used.
static void intControlC(int value)
{
    gExitApp = true;

    // reset the control-c interrupt so it 
    // can be used as a forced exit
    signal(SIGINT, SIG_DFL);
}

int32_t parseCommandLine(int arge, char *argv[])
{
    // if asking for help, exit with default error
    if(arge == 2 && (strcmp(argv[1], "-h") == 0 ||
                     strcmp(argv[1], "--help") == 0)) {
        return NOT_ENOUGH_ARGUMENTS;
    }

    // Making sure we have the correct number of arguments
    if (!(arge == 4 || 
          arge == 5)) {
        printf("Not enough command line arguments.\n");
        return NOT_ENOUGH_ARGUMENTS;
    }

    // TTY port for cellular module UART communication
    if (strlen(argv[1]) > MAX_TTY_UART_NAME) {
        return TTY_UART_NAME_TOO_BIG;
    }

#ifdef BUILD_TARGET_RASPBERRY_PI
    /* TTY UART CONNECTION FOR THE CELLULAR MODULE */
    memset(ttyUART, 0, sizeof(ttyUART));
    strcpy(ttyUART, argv[1]);
#endif
#ifdef BUILD_TARGET_WINDOWS
    char *ptr;
    comPortNumber = strtol(argv[1], &ptr, 10);
#endif

    /* CELLULAR MODULE */
    //TOLOWER(argv[2]);
    if (strcmp(argv[2], "SARA-U201") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_U201;
    else if (strcmp(argv[2], "SARA-R5") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R5;
    else if (strcmp(argv[2], "SARA-R422") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R422;
    else if (strcmp(argv[2], "SARA-R412m-03b") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R412M_03B;
    else if (strcmp(argv[2], "SARA-R412m-02b") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R412M_02B;
    else if (strcmp(argv[2], "SARA-R410m-03b") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R410M_03B;
    else if (strcmp(argv[2], "SARA-R410m-02b") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_SARA_R410M_02B;
    else if (strcmp(argv[2], "LARA-R6") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_LARA_R6;
    else if (strcmp(argv[2], "LENA-R8") == 0)
        cellModuleType = U_CELL_MODULE_TYPE_LENA_R8;
    else {
        printf("Unsupported Cellular module type: '%s'\n", argv[2]);
        return UNSUPPORTED_CELL_MODULE;
    }

    /* GNSS MODULE */
    //TOLOWER(argv[3]);
    if (strcmp(argv[3], "M8") == 0)
        gnssModuleType = U_GNSS_MODULE_TYPE_M8;
    else if (strcmp(argv[3], "M9") == 0)
        gnssModuleType = U_GNSS_MODULE_TYPE_M9;
    else if (strcmp(argv[3], "M10") == 0)
        gnssModuleType = U_GNSS_MODULE_TYPE_M10;
    else {
        printf("Unsupported GNSS module type: '%s'\n", argv[3]);
        return UNSUPPORTED_GNSS_MODULE;
    }

    // Configuration file option
    memset(configFileName, 0, sizeof(configFileName));
    if (arge == 5) {
        if (strlen(argv[4]) > MAX_CONFIG_FILENAME) {
            return CONFIG_FILENAME_TOO_BIG;
        }
        strcpy(configFileName, argv[4]);
    } else {
        strcpy(configFileName, CONFIGURATION_FILENAME);
    }

    return 0;
}

void displayHelp(int32_t errCode)
{
    if (errCode == UNSUPPORTED_CELL_MODULE) {
        printf("Supported Cellular <CellModuleType>:-\n");
        printf("\tSARA-U201\n");
        printf("\tSARA-R5\n");
        printf("\tSARA-R422\n");
        printf("\tSARA-R412M-03B\n");
        printf("\tSARA-R412M-02B\n");
        printf("\tSARA-R410M-03B\n");
        printf("\tSARA-R410M-02B\n");
        printf("\tLARA-R6\n");
        printf("\tLENA-R8\n\n");
    } else if (errCode == UNSUPPORTED_GNSS_MODULE) {
        printf("Supported GNSS <GnssModuleType>:-\n");
        printf("\tM8\n");
        printf("\tM9\n");
        printf("\tM10\n\n");    
    } else if (errCode == TTY_UART_NAME_TOO_BIG) {
        printf("<ttyDevice> name is too long. Must be 20 characters or less.\n\n");
    } else {
        displayAppVersion();
#ifdef BUILD_TARGET_RASPBERRY_PI
        printf("Use the command line arguments <ttyDevice> <CellModuleType> <GnssModuleType> [config]\n");
        printf("\n");
        printf("   ./cellular_tracker /dev/ttyUSB0 SARA-R510 M8S\n");
#endif
#ifdef BUILD_TARGET_WINDOWS
        printf("Use the command line arguments <Com port Number> <CellModuleType> <GnssModuleType> [config]\n");
        printf("\n");
        printf("   ./cellular_tracker 27 SARA-R510 M8S\n");
#endif

        printf("\nConfiguration file is optional at the end.\n\n");
    }
}

/* ----------------------------------------------------------------
 * Main starting point of the application.
 * -------------------------------------------------------------- */
int main(int arge, char *argv[])
{

    int32_t errCode;    
    if ((errCode = parseCommandLine(arge, argv)) != 0) {
        displayHelp(errCode);
        return -1;
    }

    if (!startupFramework())
        return -1;

    // The Network registration task is used to connect to the cellular network
    // This will monitor the +CxREG URCs
    if (runTask(NETWORK_REG_TASK, networkIsUp) != U_ERROR_COMMON_SUCCESS) goto FINALIZE;

    // The MQTT task connects and reconnects to the MQTT broker selected in the 
    // config.h file. This needs to run for MQTT messages to be published and
    // for remote control messages to be handled
    if (runTask(MQTT_TASK, mqttConnectionIsUp) != U_ERROR_COMMON_SUCCESS) goto FINALIZE;

    // Subscribe to the main AppControl topic for remote control the main application (this)
    subscribeToTopicAsync(APP_CONTROL_TOPIC, U_MQTT_QOS_AT_MOST_ONCE, callbacks, NUM_ELEMENTS(callbacks));

    signal(SIGINT, intControlC);
    printDebug("Control-C now hooked");

    printDebug("Application Loop now starting");

    // Start the application loop with our app function
    runApplicationLoop(appFunction);

FINALIZE:
    // all done, close down and finalize
    finalize(SHUTDOWN);

    return 0;
}
