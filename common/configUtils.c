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
 * Utility functions to help load and save 'config' files
 *
 */

#include "common.h"
#include "fileSystem.h"

#ifdef BUILD_TARGET_WINDOWS
#include "../ubxlib/port/platform/windows/src/u_port_clib_platform_specific.h" 
                                            /* strtok_r and integer stdio, must
                                            be included before the other port
                                            files if any print or scan function
                                            is used. */
#endif

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define FILE_READ_BUFFER 50

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef struct APP_CONFIG_LIST {
    char *key;
    char *value;
    struct APP_CONFIG_LIST *pNext;
} appConfigList_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static FILE *configFile;

// This contains the loaded configuration file, which the configList points to.
static char *configText = NULL;

// This is a simple linked list which is used to navigate through the configText data.
static appConfigList_t *configList = NULL;

/* ----------------------------------------------------------------
 * STATIC PRIVATE FUNCTIONS
 * -------------------------------------------------------------- */
static appConfigList_t *createConfigKVP(char *key, char *value) {
    appConfigList_t *newKVP = (appConfigList_t *)pUPortMalloc(sizeof(appConfigList_t));
    if (newKVP == NULL) {
        writeError("Failed to allocate memory for new KeyValuePair config parameter");
        return NULL;
    }

    // This sets pointers to the content in the configText static variable.
    newKVP->key = key;
    newKVP->value = value;
    newKVP->pNext = NULL;

    return newKVP;
}

static size_t parseConfiguration()
{
    size_t count = 0;
    appConfigList_t *current, *newNode;
    appConfigList_t **head = &configList;

    char *pSaveCfg;

    // Use strtok_r to tokenize each line
    char *line = strtok_r((char *)configText, "\n", &pSaveCfg);
    while (line != NULL) {
        // Skip comments
        if (line[0] == '#') {
            line = strtok_r(NULL, "\n", &pSaveCfg);
            continue;
        }

        // Find the position of " " (space)
        char *separator = strstr(line, " ");

        if (separator != NULL) {
            // Extract key and value
            *separator = '\0';  // Replace separator ' ' with null terminator
            char *key = line;
            char *value = separator + 1;  // Skip ' '(space) and point to the value

            newNode = createConfigKVP(key, value);
            if (newNode == NULL)
                break;

            if(count == 0) {
                *head = newNode;
                current = *head;
            } else {
                current->pNext = newNode;
                current = current->pNext;
            }

            count++;
        }

        // Move to the next line
        line = strtok_r(NULL, "\n", &pSaveCfg);
    }

    return count;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Loads a configuration file ready for indexing. Can load multiple config files
/// @param filename The filename of the configuration file
/// @return 0 on success, negative on failure
int32_t loadConfigFile(const char *filename)
{
    int32_t success;
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    if (filename == NULL) {
        writeFatal("Invalid configuration file: NULL");
        errorCode = U_ERROR_COMMON_NOT_INITIALISED;
    }

    printInfo("Loading configuration: %s", filename);

    const char *path = fsPath(filename);
    if (!fsFileExists(path)) {
        writeFatal("Configuration file not found: %s", filename);
        errorCode = U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    int32_t fileSize;
    success = fsFileSize(path, &fileSize);
    if (!success) {
        writeError("Failed to get filesize of configuration file '%s': %d", filename, success);
        errorCode =  U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    configText = (char *)pUPortMalloc((size_t) fileSize);
    if (configText == NULL) {
        writeError("Failed to allocate memory for loading in configuration file, size: %d", fileSize);
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanUp;
    }

    fsInit(&configFile);
    configFile = fsOpenRead(path);
    if (configFile == NULL) {
        writeError("Failed to open configuration file: %d", success);
        errorCode = U_ERROR_COMMON_NOT_FOUND;
        goto cleanUp;
    }

    size_t count;
    char *buffer = configText;
    while((count = fsRead(buffer, FILE_READ_BUFFER, configFile)) > 0) {
        buffer = buffer + count;
    }

    parseConfiguration();

cleanUp:
    if (errorCode != 0) {
        uPortFree(configText);
    }

    fsClose(configFile);

    return errorCode;
}

void printConfiguration(void)
{
    size_t count=1;
    appConfigList_t *kvp = configList;
    const char *value;

    while(kvp != NULL) {
        value = getConfig(kvp->key);
        if (value == NULL) value = "N/A";
        printDebug("   Key #%d: %s = %s", count, kvp->key, value);

        kvp = kvp->pNext;
        count++;
    }

    printDebug("");
}

/// @brief Returns the specified configuration value
/// @param key The configuration name to return the value of
/// @return The configuration value on success, NULL on failure
const char *getConfig(const char *key)
{
    for(appConfigList_t *kvp = configList; kvp != NULL; kvp=kvp->pNext) {
        if (strcmp(kvp->key, key) == 0) {
            if (strncmp(kvp->value, "NULL", 4) == 0)
                return NULL;
            else
                return kvp->value;
        }
    }

    printWarn("Failed to find '%s' key", key);
    return NULL;
}

/// @brief Sets an int value from a configuration key, if present
/// @param key The configuration name to return the value of
/// @param param A pointer to the int value to set
/// @return True if the int value was set, False otherwise
bool setIntParamFromConfig(const char *key, int32_t *param)
{
    const char *value = getConfig(key);
    if (value == NULL) return false;

    *param = atoi(value);

    return true;
}

/// @brief Sets a bool value from a configuration key, if present
/// @param key The configuration name to return the value of
/// @param compare The string to compare the config value to
/// @param param A pointer to the bool value to set
/// @return True if the bool value was set, False otherwise
bool setBoolParamFromConfig(const char *key, const char *compare, bool *param)
{
    const char *value = getConfig(key);
    if (value == NULL) return false;

    *param = strcmp(value, compare) == 0;
    return true;
}

/// @brief frees memory from the configuration malloced array
void closeConfig(void)
{
    appConfigList_t *pCfg = configList;
    for(appConfigList_t *kvp = configList; kvp != NULL && pCfg != NULL;) {
        kvp=pCfg->pNext;
        uPortFree(pCfg);
        pCfg = kvp;
    }

    uPortFree(configText);
}