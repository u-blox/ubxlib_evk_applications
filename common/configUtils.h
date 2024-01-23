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
 * Configuration file utility functions
 *
 */

#ifndef _CONFIG_UTILS_H_
#define _CONFIG_UTILS_H_

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief          Returns the number of configuration items loaded
/// @return         The number of configuration items loaded
size_t getConfigItemCount(void);

/// @brief          Loads a configuration file ready for indexing
/// @param filename The filename of the configuration file
/// @return         0 on success, negative on failure
int32_t loadConfigFile(const char *filename);

/// @brief          Parses the loaded configuration text file
/// @return         0 on success, negative on failure
int32_t parseConfiguration(void);

/// @brief          Prints the configuration items
void printConfiguration(void);

/// @brief          Returns the specified configuration value
/// @param key      The configuration name to return the value of
/// @return         The configuration value on succes, NULL on failure
const char *getConfig(const char *key);

/// @brief          Sets a int value from a configuration key, if present
/// @param key      The configuration name to return the value of
/// @param param    A pointer to the int value to set
/// @return         True if the int value was set, False otherwise
bool setIntParamFromConfig(const char *key, int32_t *param);

/// @brief          Sets a bool value from a configuration key, if present
/// @param key      The configuration name to return the value of
/// @param param    A pointer to the bool value to set
/// @return         True if the bool value was set, False otherwise
bool setBoolParamFromConfig(const char *key, const char *value, bool *param);

/// @brief          Clears down the memory allocated by the configuration
void closeConfig(void);

#endif