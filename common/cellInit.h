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

#ifndef _CELLINIT_H_
#define _CELLINIT_H_

/// @brief Configures the cellular module according to the config settings
/// @return 0 on success, negative on failure
int32_t configureCellularModule(void);

/// @brief Gets the cellular module information
void getCellularModuleInfo(void);

/// @brief Publishes the cellular module information
/// @param networkBackUpCounter     The number of times the network has come back up
/// @return 0 on sucess, negative on failure
int32_t publishCellularModuleInfo(int32_t networkBackUpCounter);
#endif