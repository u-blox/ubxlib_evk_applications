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

#include <stdio.h>
#include <stdbool.h>

/**
 * Initiate the file system
 * @return  Success or failure.
 */
bool fsInit(void *fptr);

/**
 * Get the full file system path including mount point name.
 * @return  Pointer to the full path.
 */
const char *fsPath(const char *fileName);

/**
 * Get the size of the free space on the file system in kB.
 * @return Actual free size or possible negative error code
 */
int32_t fsFree();

/**
 * Check if a file exists
 * @param   filePath  Complete file name path.
 * @return            True if the file exists.
 */
bool fsFileExists(const char *fileName);

/**
 * Get the size of file in bytes
 * @param   filePath  Complete file name path
 * @param   size      Place to put the size
 * @return            True if the file exists
 *                    and size could be read.
 */
bool fsFileSize(const char *fileName, int32_t *size);

/**
 * Open the file for reading/writing
 * @param   filePath    Complete file name path
 * @return              The pointer to the file
*/
FILE *fsOpenRead(const char *filename);

/**
 * Open the file for writing. Will create the file
 * if it does not exist.
 * @param   filename    The name of the file
 * @return              The returned file pointer
*/
FILE *fsOpenWrite(const char *filename);

/***
 * Close the file
 * @param  fptr         The pointer to the file to close.
 * @return              True if successful.
*/
bool fsClose(FILE *fptr);

/**
 * Write data to the file
*/
size_t fsWrite(const char *data, size_t size, FILE *fptr);

/**
 * Read data from the file
*/
size_t fsRead(char *data, size_t size, FILE *fptr);

/***
 * Delete the file
*/
bool fsDelete(const char *filename);