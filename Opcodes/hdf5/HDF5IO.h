/*
 * Copyright (C) 2014 Edward Costello
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "csdl.h"
#include "hdf5.h"
#include <stdbool.h>

typedef enum ArgumentType
{
    STRING_VAR,
    ARATE_VAR,
    KRATE_VAR,
    IRATE_VAR,
    ARATE_ARRAY,
    KRATE_ARRAY,
    IRATE_ARRAY,
    UNKNOWN
} ArgumentType;

typedef struct _fft {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in, *in2;
    MYFLT *f;
    MYFLT b;
    int32_t n;
    void *setup;
    AUXCH mem;
} nFFT;



typedef struct HDF5Dataset
{
    char *datasetName;
    AUXCH datasetNameMemory;
    void *argumentPointer;
    ArgumentType writeType;
    ArgumentType readType;
    int32_t rank;
    hsize_t *chunkDimensions;
    AUXCH chunkDimensionsMemory;
    hsize_t *maxDimensions;
    AUXCH maxDimensionsMemory;
    hsize_t *offset;
    AUXCH offsetMemory;
    hsize_t *datasetSize;
    AUXCH datasetSizeMemory;

    hid_t datasetID;

    size_t elementCount;
    MYFLT *sampleBuffer;
    AUXCH sampleBufferMemory;

    bool readAll;

} HDF5Dataset;

typedef struct HDF5File
{
    hid_t fileHandle;
    char *fileName;
    hid_t floatSize;

} HDF5File;


HDF5File *HDF5IO_newHDF5File(CSOUND *csound, AUXCH *hdf5FileMemory,
                             STRINGDAT *path, bool openForWriting);

void HDF5IO_deleteHDF5File(CSOUND *csound, HDF5File *hdf5File);

typedef struct HDF5Write
{
    OPDS h;
    MYFLT *arguments[20];
    int32_t inputArgumentCount;
    size_t ksmps;
    HDF5File *hdf5File;
    AUXCH hdf5FileMemory;
    HDF5Dataset *datasets;
    AUXCH datasetsMemory;

} HDF5Write;

int32_t HDF5Write_initialise(CSOUND *csound, HDF5Write *self);

int32_t HDF5Write_process(CSOUND *csound, HDF5Write *self);

int32_t HDF5Write_finish(CSOUND *csound, void *inReference);

void HDF5Write_checkArgumentSanity(CSOUND *csound, const HDF5Write *self);

void HDF5Write_createDatasets(CSOUND *csound, HDF5Write *self);

void HDF5Write_newArrayDataset(CSOUND *csound, HDF5Write *self,
                               HDF5Dataset *dataset);

void HDF5Write_deleteArrayDataset(CSOUND *csound, HDF5Dataset *dataset);


typedef struct HDF5Read
{
    OPDS h;
    MYFLT *arguments[20];
    int32_t inputArgumentCount;
    int32_t outputArgumentCount;
    size_t ksmps;
    HDF5File *hdf5File;
    AUXCH hdf5FileMemory;
    HDF5Dataset *datasets;
    AUXCH datasetsMemory;
    bool isSampleAccurate;

} HDF5Read;

int32_t HDF5Read_initialise(CSOUND *csound, HDF5Read *self);

int32_t HDF5Read_process(CSOUND *csound, HDF5Read *self);

int32_t HDF5Read_finish(CSOUND *csound, void *inReference);

void HDF5Read_checkArgumentSanity(CSOUND *csound, const HDF5Read *self);

void HDF5Read_openDatasets(CSOUND *csound, HDF5Read *self);
