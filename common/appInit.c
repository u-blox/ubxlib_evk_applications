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

#include "common.h"
#include "taskControl.h"
#include "cellInit.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define STARTUP_DELAY 250       // 250 * 20ms => 5 seconds
#define LOG_FILENAME "log.csv"

// Dwell time of the main loop activity, pause period until the loop runs again
#define APP_DWELL_TIME_MS_MINIMUM 5000
#define APP_DWELL_TIME_MS_DEFAULT APP_DWELL_TIME_MS_MINIMUM;
#define APP_DWELL_TICK_MS 50

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uDeviceCfg_t deviceCfg;

// Command line argument <cellModuleType> which represents the u_cell_module_type.h value
extern int32_t cellModuleType;

// Command line argument <gnssModuleType> which represents the u_cell_module_type.h value
extern char ttyUART[];

// Command line argument [config] which represents the configuration file to load.
// If not specified then the configuration file will be CONFIGURATION_FILENAME default.
extern char configFileName[];

static int32_t appDwellTimeMS = 5000;

// This flag will pause the main application loop
static bool pauseMainLoopIndicator = false;

static bool appFinalized = false;

static int32_t exitCode = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */
applicationStates_t gAppStatus = MANUAL;

// deviceHandle is not static as this is shared between other modules.
uDeviceHandle_t gCellDeviceHandle;

// This flag is set to true when the application should close tasks and log files.
// This flag is set to true when Button #1 is pressed.
bool gExitApp = false;

void (*buttonTwoFunc)(void) = NULL;

/* ----------------------------------------------------------------
 * EXTERNAL GLOBAL VARIABLES
 * -------------------------------------------------------------- */
// reference to our mqtt credentials which are used for the application's publish/subscription
extern const char *mqttCredentials[];
extern int32_t mqttCredentialsSize;

#ifdef BUILD_TARGET_WINDOWS
extern int32_t comPortNumber;
#endif

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void configCellModule(uDeviceCfg_t *pDeviceCfg)
{
    pDeviceCfg->deviceType = U_DEVICE_TYPE_CELL;
    pDeviceCfg->transportType = U_DEVICE_TRANSPORT_TYPE_UART;

    pDeviceCfg->deviceCfg.cfgCell.moduleType = cellModuleType;
    // Raspberry PI build doesn't use the PINs [yet]
    pDeviceCfg->deviceCfg.cfgCell.pinDtrPowerSaving = -1;
    pDeviceCfg->deviceCfg.cfgCell.pinEnablePower = -1;
    pDeviceCfg->deviceCfg.cfgCell.pinPwrOn = -1;
    pDeviceCfg->deviceCfg.cfgCell.pinVInt = -1;

#ifdef BUILD_TARGET_RASPBERRY_PI
    // Raspberry PI build, we set the .uart to -1 to
    // just use the pPrefix, which will be something
    // like '/dev/ttyUSB0' or '/dev/ttyEVK'
    pDeviceCfg->transportCfg.cfgUart.uart = -1;
    pDeviceCfg->transportCfg.cfgUart.pPrefix = ttyUART;
#endif
#ifdef BUILD_TARGET_WINDOWS
    // Windows build, we set the .uart to the port number
    pDeviceCfg->transportCfg.cfgUart.uart = comPortNumber;
#endif

    pDeviceCfg->transportCfg.cfgUart.baudRate = U_CELL_UART_BAUD_RATE;

    // Raspberry PI build doesn't use Flow control as it's over USB
    pDeviceCfg->transportCfg.cfgUart.pinCts = -1;
    pDeviceCfg->transportCfg.cfgUart.pinRts = -1;
    pDeviceCfg->transportCfg.cfgUart.pinRxd = -1;
    pDeviceCfg->transportCfg.cfgUart.pinTxd = -1;
}

/// @brief Initiate the UBXLIX API
static int32_t initCellularDevice(void)
{
    int32_t errorCode;

    // turn off the UBXLIB printInfo() logging as it is enabled by default (?!)
    #ifndef UBXLIB_LOGGING_ON
        printDebug("UBXLIB Logging is turned off.");
        uPortLogOff();
    #else
        printDebug("UBXLIB Logging is turned ON");
    #endif

    writeInfo("Initiating the UBXLIB Device API...");
    errorCode = uDeviceInit();
    if (errorCode != 0) {
        writeDebug("* uDeviceInit() Failed: %d", errorCode);
        return errorCode;
    }

    configCellModule(&deviceCfg);

    printDebug("Cell Cfg - Module type: %d", deviceCfg.deviceCfg.cfgCell.moduleType);
    printDebug("Cell Cfg -   Transport: %d", deviceCfg.transportType);
#ifdef BUILD_TARGET_RASPBERRY_PI
    printDebug("Cell Cfg -   UART name: %s", deviceCfg.transportCfg.cfgUart.pPrefix);
#endif
#ifdef BUILD_TARGET_WINDOWS
    printDebug("Cell Cfg -   UART name: COM%d", deviceCfg.transportCfg.cfgUart.uart);
#endif

    writeInfo("Opening/Turning on the cellular module...");
    errorCode = uDeviceOpen(&deviceCfg, &gCellDeviceHandle);
    if (errorCode < 0) {
        writeFatal("* Failed to turn on the cellular module with uDeviceOpen(): %d", errorCode);
#ifdef BUILD_TARGET_WINDOWS
        if (errorCode == U_ERROR_COMMON_PLATFORM)
            writeInfo("Is COM%d already being used?", deviceCfg.transportCfg.cfgUart.uart);
#endif
        return errorCode;
    }

    gAppStatus = INIT_DEVICE_DONE;
    getCellularModuleInfo();

    return configureCellularModule();
}

/// @brief Dwells for appDwellTimeMS time, and exits if this time changes or Exit App
static void dwellAppLoop(void)
{
    int32_t tick = 0;
    int32_t previousDwellTimeMS = appDwellTimeMS;

    do
    {
        uPortTaskBlock(APP_DWELL_TICK_MS);
        tick = tick + APP_DWELL_TICK_MS;
    } while((tick < previousDwellTimeMS) &&
            (previousDwellTimeMS == appDwellTimeMS) &&
            (!gExitApp));
}

static int32_t closeCellularDevice(void)
{
    writeInfo("Turning off Cellular Module...");
    int32_t errorCode;
    
    bool powerOff = false;

    if (gCellDeviceHandle == NULL) {
        writeDebug("closeCellularDevice(): Cellular module handle is NULL");
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    errorCode = uDeviceClose(gCellDeviceHandle, powerOff);
    if (errorCode < 0) {
        writeWarn("Failed to close the cellular module with uDeviceClose(): %d", errorCode);
        return errorCode;
    }

    return U_ERROR_COMMON_SUCCESS;
}

static int32_t deinitUbxlibDevices(void)
{
    int32_t errorCode;
    
    errorCode = uDeviceDeinit();
    if (errorCode < 0) {
        writeWarn("Failed to de-initialize the device API with uDeviceDeinit(): %d", errorCode);
        return errorCode;
    }

    uPortDeinit();

    return U_ERROR_COMMON_SUCCESS;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Sets the time between each main loop execution
/// @param params The dwell time parameter for the dwell time
/// @return 0 if successful, or failure if invalid parameters
int32_t setAppDwellTime(commandParamsList_t *params)
{
    int32_t timeMS = getParamValue(params, 1, 5000, 60000, 30000);

    if (timeMS < APP_DWELL_TIME_MS_MINIMUM) {
        writeWarn("Failed to set App Dwell Time, %d is less than minimum (%d ms)", timeMS, APP_DWELL_TIME_MS_MINIMUM);
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    appDwellTimeMS = timeMS;
    writeInfo("Setting App Dwell Time to: %d\n", timeMS);

    return U_ERROR_COMMON_SUCCESS;
}

/// @brief Sets the application logging level
/// @param params The log level parameter for the dwell time
/// @return 0 if successful, or failure if invalid parameters
int32_t setAppLogLevel(commandParamsList_t *params)
{
    logLevels_t logLevel = (logLevels_t) getParamValue(params, 1, (int32_t) eTRACE, (int32_t) eMAXLOGLEVELS, (int32_t) eINFO);

    if (logLevel < eTRACE) {
        writeWarn("Failed to set App Log Level %d. Min: %d, Max: %d", logLevel, eTRACE, eMAXLOGLEVELS);
        return U_ERROR_COMMON_INVALID_PARAMETER;
    }

    setLogLevel(logLevel);

    return U_ERROR_COMMON_SUCCESS;
}

int32_t exitApplication(commandParamsList_t *params)
{
    exitCode = getParamValue(params, 1, -10, 10, 0);
    printWarn("Application exiting with code: %d", exitCode);

    gExitApp = true;

    return U_ERROR_COMMON_SUCCESS;
}

/// @brief Method of pausing the running of the main loop. 
///        Useful for when there are other long running activities
///        which need to stop the main loop
/// @param state A flag to indicate if the main loop is paused or not
void pauseMainLoop(bool state)
{
    pauseMainLoopIndicator = state;
    printInfo("Main application loop %s", state ? "is paused" : "is unpaused");
}

/// @brief This is the main application loop which runs the appFunc which is 
///        defined in the application main.c module.
/// @param appFunc The function pointer of the app event code
void runApplicationLoop(bool (*appFunc)(void))
{
    printDebug("Application Loop now starting");
    while(!gExitApp) {
        printDebug("*** Application Tick ***\n");
        dwellAppLoop();

        if (gExitApp) return;

        if (pauseMainLoopIndicator) {
            writeDebug("Application loop paused.");
            continue;
        }

        if (!appFunc()) {
            gExitApp = true;
            writeInfo("Application function stopped the app loop");
        }
    }
}

/// @brief Sets the application status, waits for the tasks and closes the log
/// @param appState The application status to set for the shutdown
void finalize(applicationStates_t appState)
{
    gAppStatus = appState;
    gExitApp = true;

    waitForAllTasksToStop();

    // now stop the network registration task.
    if (stopAndWait(NETWORK_REG_TASK, 15) < 0 && appState != ERROR)
        printWarn("Did not stop the registration task properly");

    finalizeAllTasks();

    closeCellularDevice();

    deinitUbxlibDevices();

    closeConfig();

    printf("\n\n\nApplication finished.\n");

    if (appState == ERROR && exitCode == 0)
        exit(-1);
    else
        exit(exitCode);
}

void displayAppVersion(void)
{
    writeInfo("*************************************************");
    writeInfo("%s %s", APP_NAME, APP_VERSION);
    writeInfo("*************************************************\n");
}

/// @brief Starts the application framework
/// @return true if successful, false otherwise
bool startupFramework(void)
{
    int32_t errorCode;

    errorCode = uPortInit();
    if (errorCode < 0) {
        printFatal("* uPortInit() Failed: %d - not running application!", errorCode);
        return false;
    }

    setLogLevel(LOGGING_LEVEL);
    startLogging(LOG_FILENAME);
    
    displayAppVersion();

    if (loadConfigFile(configFileName) < 0)
        return false;
    
    printConfiguration();

    // initialise the cellular module
    gAppStatus = INIT_DEVICE;
    errorCode = initCellularDevice();
    if (errorCode != 0) {
        printInfo("Can't continue running the application.");
        finalize(ERROR);
    }

    // Initialise the task runners
    printDebug("ubxlib Port and cellular device is ready.");
    printDebug("Initialising the application Tasks");
    if (initTasks() != 0) {
        finalize(ERROR);
    }

    return true;
}