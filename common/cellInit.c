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
#include "configUtils.h"
#include "cellInit.h"

#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define INFO_BUFFER_SIZE 50

/* ----------------------------------------------------------------
 * GLOBAL VARIABLES
 * -------------------------------------------------------------- */
// Module information
char gModuleSerial[U_CELL_INFO_IMEI_SIZE+1];
char gModuleManufacturer[INFO_BUFFER_SIZE];
char gModuleModel[INFO_BUFFER_SIZE];
char gModuleVersion[INFO_BUFFER_SIZE];

// SIM information
char gIMSI[U_CELL_INFO_IMSI_SIZE+1];
char gCCID[U_CELL_INFO_IMSI_SIZE+1];

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static char topicName[MAX_TOPIC_NAME_SIZE];

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static int32_t checkReboot(void)
{
    if(uCellPwrRebootIsRequired(gCellDeviceHandle))
    {
        writeInfo("Need to reboot the module as the settings have changed...");
        return uCellPwrReboot(gCellDeviceHandle, NULL);
    }

    return 0;
}

static int32_t configureMNOProfile(void)
{
    int32_t cfgMNOProfile;
    if (!setIntParamFromConfig("MNOPROFILE", &cfgMNOProfile)) {
        writeError("Failed to get the MNO_PROFILE setting from the configuration file.");
        return U_ERROR_COMMON_NOT_FOUND;
    }

    int32_t errorCode = uCellCfgGetMnoProfile(gCellDeviceHandle);
    if (errorCode < 0) {
        writeError("configureMNOProfile(): Failed to get MNO Profile: %d", errorCode);
        return errorCode;
    }

    if (errorCode == cfgMNOProfile) {
        writeDebug("MNO Profile already set to %d", cfgMNOProfile);
        return 0;
    }

    writeDebug("Need to set MNO Profile from %d to %d", errorCode, cfgMNOProfile);
    errorCode = uCellCfgSetMnoProfile(gCellDeviceHandle, cfgMNOProfile);
    if(errorCode != 0)
    {
        writeError("configureMNOProfile(): Failed to set MNO Profile %d", cfgMNOProfile);
        return errorCode;
    }

    return checkReboot();
}

static int32_t configureRAT(void)
{
    int32_t cfgRAT;
    if (!setIntParamFromConfig("URAT", &cfgRAT)) {
        writeError("Failed to get the URAT setting from the configuration file.");
        return U_ERROR_COMMON_NOT_FOUND;
    }

    int32_t rat = uCellCfgGetRat(gCellDeviceHandle, 0);
    if (rat == cfgRAT) {
        writeDebug("URAT already set to %d", rat);
        return 0;
    }

    writeDebug("Need to set URAT from %d to %d", rat, cfgRAT);
    int32_t errorCode = uCellCfgSetRat(gCellDeviceHandle, cfgRAT);
    if (errorCode != 0)
    {
        writeError("Failed to set RAT %d", cfgRAT);
        return errorCode;
    }

    return checkReboot();
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
int32_t configureCellularModule(void)
{
    writeInfo("Configuring the cellular module...");

    int32_t errorCode = configureMNOProfile();
    if (errorCode == 0)
        errorCode = configureRAT();

    return errorCode;
}

/// @brief Get the cellular module's information 
void getCellularModuleInfo(void)
{
    int32_t count;
    int32_t errorCode;

    count = uCellInfoGetManufacturerStr(gCellDeviceHandle, gModuleManufacturer, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Manufacturer: %s", gModuleManufacturer);
    else
        writeWarn("Cellular Module Manufacturer: Failed to get");

    count = uCellInfoGetModelStr(gCellDeviceHandle, gModuleModel, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Model: %s", gModuleModel);
    else
        writeWarn("Cellular Module Model: Failed to get");

    count = uCellInfoGetFirmwareVersionStr(gCellDeviceHandle, gModuleVersion, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module Version: %s", gModuleVersion);
    else
        writeWarn("Cellular Module Version: Failed to get");

    // getIMxI functions return fixed chars, not a null terminated string (!)
    gIMSI[sizeof(gIMSI)-1] = 0;
    gModuleSerial[sizeof(gModuleSerial)-1] = 0;

    errorCode = uCellInfoGetImei(gCellDeviceHandle, gModuleSerial);
    if (errorCode == 0)
        writeInfo("Cellular Module IMEI: %s", gModuleSerial);
    else
        writeWarn("Cellular Module IMEI: Failed to get: %d", errorCode);

    errorCode = uCellInfoGetImsi(gCellDeviceHandle, gIMSI);
    if (errorCode == 0)
        writeInfo("Cellular Module IMSI: %s", gIMSI);
    else
        writeWarn("Cellular Module IMSI: Failed to get: %d", errorCode);

    count = uCellInfoGetIccidStr(gCellDeviceHandle, gCCID, INFO_BUFFER_SIZE);
    if (count > 0)
        writeInfo("Cellular Module CCID: %s", gCCID);
    else
        writeWarn("Cellular Module CCID: Failed to get: %d", errorCode);
}

/// @brief Publishes the module information acquired by getCellularModuleInfo()
/// @param networkBackUpCounter     counter for how many times the network has come back "up"
/// over the main module's MQTT 'Information' topic
void publishCellularModuleInfo(int networkBackUpCounter)
{
    char timestamp[TIMESTAMP_MAX_LENGTH_BYTES];
    getTimeStamp(timestamp);

    char format[] = "{"                     \
            "\"Timestamp\":\"%s\", "        \
            "\"Module\":{"                  \
                "\"Manufacturer\":\"%s\", " \
                "\"Model\":\"%s\", "        \
                "\"Version\":\"%s\"},"      \
            "\"SIM\":{"                     \
                "\"IMSI\":\"%s\", "         \
                "\"CCID\":\"%s\"},"         \
            "\"Application\":{"             \
                "\"NetworkUpCounter\":%d}"  \
        "}";

    char jsonBuffer[300];
    snprintf(jsonBuffer, 300, format, timestamp,
            gModuleManufacturer,
            gModuleModel,
            gModuleVersion,
            gIMSI, gCCID,
            networkBackUpCounter);

    snprintf(topicName, MAX_TOPIC_NAME_SIZE, "%s/%s", (const char *)gModuleSerial, "Information");
    publishMQTTMessage(topicName, jsonBuffer, U_MQTT_QOS_AT_MOST_ONCE, true);
    writeAlways(jsonBuffer);
}