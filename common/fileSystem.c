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

#include <stdio.h>
#include "fileSystem.h"

#include <sys/stat.h>

bool fsInit(void *fptr)
{
    return true;
}

const char *fsPath(const char *filename)
{
    return filename;
}

bool fsFileExists(const char *filename)
{    
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}

bool fsFileSize(const char *filename, int32_t *size)
{
    struct stat buffer;
    int success = stat(filename, &buffer);

    *size = (int32_t) buffer.st_size;

    return success == 0;
}

/**
 * Open the file for writing
 * @param   filePath  Complete file name path
  * @return            The pointer to the file
*/
FILE *fsOpenWrite(const char *filename)
{
    return fopen(filename, "w");
}

/**
 * Open the file for reading
 * @param   filePath    Complete file name path
  * @return             The pointer to the file
*/
FILE *fsOpenRead(const char *filename)
{
    return fopen(filename, "r");
}

/**
 * Write data to the file
*/
size_t fsWrite(const char *data, size_t size, FILE *fptr)
{
    return fwrite((void *)data, 1, size, fptr);
}

/**
 * Read data from the file
*/
size_t fsRead(char *data, size_t size, FILE *fptr)
{
    return fread(data, 1, size, fptr);
}

bool fsDelete(const char *filename)
{
    return remove(filename) == 0;
}

bool fsClose(FILE *fptr)
{
    if (fptr == 0)
        return true;

    return fclose(fptr) == 0;
}