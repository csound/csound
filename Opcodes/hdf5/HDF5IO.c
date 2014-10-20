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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#import "HDF5IO.h"
#import <string.h>
#import <unistd.h>

#define HDF5ERROR(x) if ((x) == -1) {csound->Die(csound, #x" error\nExiting\n");}
#pragma mark - Common -

// Type strings to match the enum types
static const char typeStrings[8][12] = {
    "STRING_VAR",
    "ARATE_VAR",
    "KRATE_VAR",
    "IRATE_VAR",
    "ARATE_ARRAY",
    "KRATE_ARRAY",
    "IRATE_ARRAY",
    "UNKNOWN"
};

// Get the argument enum type from the opcode argument pointer
//
// Get the csound type from the argument
// Get the variable type name
// Do a string comparison and match the type to the enum type

ArgumentType HDF5IO_getArgumentTypeFromArgument(CSOUND *csound, MYFLT *argument)
{
    const CS_TYPE *csoundType = csound->GetTypeForArg((void *)argument);
    const char *type = csoundType->varTypeName;
    ArgumentType argumentType = UNKNOWN;
    
    if (strcmp("S", type) == 0) {
        
        argumentType = STRING_VAR;
    }
    else if (strcmp("a", type) == 0) {
        
        argumentType = ARATE_VAR;
    }
    else if (strcmp("k", type) == 0) {
        
        argumentType = KRATE_VAR;
    }
    else if (strcmp("i", type) == 0) {
        
        argumentType = IRATE_VAR;
    }
    else if (strcmp("[", type) == 0) {
        
        ARRAYDAT *array = (ARRAYDAT *)argument;
        
        if (strcmp("k", array->arrayType->varTypeName) == 0) {
            
            argumentType = KRATE_ARRAY;
        }
        else if (strcmp("a", array->arrayType->varTypeName) == 0) {
            
            argumentType = ARATE_ARRAY;
        }
        else if (strcmp("i", array->arrayType->varTypeName) == 0) {
            
            argumentType = IRATE_ARRAY;
        }
    }
    
    return argumentType;
}

// Get the matching argument enum type from a string
//
// Do a string comparison and assign the corresponding enum type to the string

ArgumentType HDF5IO_getArgumentTypeFromString(CSOUND *csound, const char *string)
{
    ArgumentType type = UNKNOWN;
    
    if (strcmp("STRING_VAR", string) == 0) {
        
        type = STRING_VAR;
    }
    else if (strcmp("ARATE_VAR", string) == 0) {
        
        type = ARATE_VAR;
    }
    else if (strcmp("KRATE_VAR", string) == 0) {
        
        type = KRATE_VAR;
    }
    else if (strcmp("IRATE_VAR", string) == 0) {
        
        type = IRATE_VAR;
    }
    else if (strcmp("ARATE_ARRAY", string) == 0) {
        
        type = ARATE_ARRAY;
    }
    else if (strcmp("KRATE_ARRAY", string) == 0) {
        
        type = KRATE_ARRAY;
    }
    else if (strcmp("IRATE_ARRAY", string) == 0) {
        
        type = IRATE_ARRAY;
    }
    
    return type;
}

// Create or open a hdf5 file
//
// Allocate the memory for a hdf5 file struct
// Assign the path string to the file name in the struct
// Check if the file exists
// If it doesn't and this function is being called by the hdf5 write function, create the file
// If it exists, open the file for appending
// Find out what size the MYFLT floating point type is and assign the correct hdf5 type to it
// Return the pointer to the hdf5 file struct

HDF5File *HDF5IO_newHDF5File(CSOUND *csound, AUXCH *hdf5FileMemory, STRINGDAT *path, bool openForWriting)
{
    csound->AuxAlloc(csound, sizeof(HDF5File), hdf5FileMemory);
    HDF5File *hdf5File = hdf5FileMemory->auxp;
    
    hdf5File->fileName = path->data;
    
    int fileExists = access(hdf5File->fileName, 0);
    
    if (fileExists == -1) {
        
        if (openForWriting == true) {
            
            hdf5File->fileHandle = H5Fcreate(hdf5File->fileName, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            HDF5ERROR(hdf5File->fileHandle);
        }
        else {
            
            csound->Die(csound, "hdf5read: Error, file doesn't exist");
        }
    }
    else {
        
        hdf5File->fileHandle = H5Fopen(hdf5File->fileName, H5F_ACC_RDWR, H5P_DEFAULT);
        HDF5ERROR(hdf5File->fileHandle);
    }
    
    if (sizeof(MYFLT) == sizeof(double)) {
        
        hdf5File->floatSize = H5T_NATIVE_DOUBLE;
    }
    else if (sizeof(MYFLT) == sizeof(float)) {
        
        hdf5File->floatSize = H5T_NATIVE_FLOAT;
    }
    else {
        
        csound->Die(csound, "HDF5IO: Illegal size for floating point type, exiting");
    }
    
    return hdf5File;
}

// Write a string attribute to a hdf5 file dataset
//
// Create a new attribute of type C string
// Set the size of the string to 11 characters, this is the longest string length of the argument type strings
// Set the string to be null terminated
// Create the attribute in the hdf5 file
// Write the string to the attribute
// Close the open handles

void HDF5IO_writeStringAttribute(CSOUND *csound, HDF5File *self, HDF5Dataset *dataset,
                                 const char *attributeName, const char *attributeString)
{
    hid_t attributeID  = H5Screate(H5S_SCALAR);
    HDF5ERROR(attributeID);
    hid_t attributeType = H5Tcopy(H5T_C_S1);
    HDF5ERROR(attributeType);
    HDF5ERROR(H5Tset_size(attributeType, 11));
    HDF5ERROR(H5Tset_strpad(attributeType,H5T_STR_NULLTERM));
    hid_t attributeHandle = H5Acreate2(dataset->datasetID, attributeName, attributeType, attributeID, H5P_DEFAULT, H5P_DEFAULT);
    HDF5ERROR(attributeHandle);
    HDF5ERROR(H5Awrite(attributeHandle, attributeType, attributeString));
    HDF5ERROR(H5Sclose(attributeID));
    HDF5ERROR(H5Tclose(attributeType));
    HDF5ERROR(H5Aclose(attributeHandle));
}

// Read a string attribute from the hdf5 file
//
// Open the data set and get the info structure
// Get the attribute at index 0, this should probably be more general, needs more work
// Get the type of the attribute
// Read the attribute into memory and close all the handles

void HDF5IO_readStringAttribute(CSOUND *csound, HDF5File *self, char *datasetName, char *attributeString)
{
    hid_t dataSetID = H5Dopen2(self->fileHandle, datasetName, H5P_DEFAULT);
    H5O_info_t oinfo;
    HDF5ERROR(H5Oget_info(dataSetID, &oinfo));
    hid_t attributeID = H5Aopen_by_idx(dataSetID, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, H5P_DEFAULT, H5P_DEFAULT);
    HDF5ERROR(attributeID);
    hid_t attributeType = H5Aget_type(attributeID);
    HDF5ERROR(attributeType);
    hid_t attributeTypeMemory = H5Tget_native_type(attributeType, H5T_DIR_ASCEND);
    HDF5ERROR(attributeTypeMemory);
    HDF5ERROR(H5Aread(attributeID, attributeTypeMemory, attributeString));
    HDF5ERROR(H5Aclose(attributeID));
    HDF5ERROR(H5Tclose(attributeType));
    HDF5ERROR(H5Tclose(attributeTypeMemory));
    HDF5ERROR(H5Dclose(dataSetID));
}

// Find if csound is running in sample accurate mode
//
// Get the argument parameters passed to run csound
// Find the sample accurate parameter and return it

bool HDF5IO_getSampleAccurate(CSOUND *csound)
{
    OPARMS parameters = {0};
    csound->GetOParms(csound, &parameters);
    return parameters.sampleAccurate;
}


#pragma mark - HDF5Write -

// Set everything up so datasets can be written from the opcodes input variables,
// i-rate datasets are written at initialisation all others are written on the performance pass.
//
// Get the amount of samples in a control pass
// Get the amount of arguments to the opcode, this doesn't include the first argument which is for the filename.
// Check that the first argument is a string for the filename, check others are not strings
// Register the callback to close the hdf5 file when performance finishes
// Get the path argument and open a hdf5 file, if it doesn't exist create it
// Create the datasets in the file so they can be written

int HDF5Write_initialise(CSOUND *csound, HDF5Write *self)
{
    self->ksmps = csound->GetKsmps(csound);
    self->inputArgumentCount = self->INOCOUNT - 1;
    HDF5Write_checkArgumentSanity(csound, self);
    csound->RegisterDeinitCallback(csound, self, HDF5Write_finish);
    
    STRINGDAT *path = (STRINGDAT *)self->arguments[0];
    self->hdf5File = HDF5IO_newHDF5File(csound, &self->hdf5FileMemory, path, true);
    HDF5Write_createDatasets(csound, self);
    
    return OK;
}

// Write the input data to the associated dataset
//
// Enlarge the dataset first then get the file space
// Select a hyperslab with the necessary offset and size
// Create a memory space where the data is written to
// Write the data to the memory space
// Close the file space

void HDF5Write_writeData(CSOUND *csound, HDF5Write *self, HDF5Dataset *dataset, MYFLT *data)
{
    HDF5ERROR(H5Dset_extent(dataset->datasetID, dataset->datasetSize));
    hid_t filespace = H5Dget_space(dataset->datasetID);
    HDF5ERROR(filespace);
    HDF5ERROR(H5Sselect_hyperslab(filespace, H5S_SELECT_SET, dataset->offset, NULL, dataset->chunkDimensions, NULL));
    hid_t memspace  = H5Screate_simple(dataset->rank, dataset->chunkDimensions, NULL);
    HDF5ERROR(memspace);
    HDF5ERROR(H5Dwrite(dataset->datasetID, self->hdf5File->floatSize, memspace, filespace, H5P_DEFAULT, data));
    HDF5ERROR(H5Sclose(filespace));
}

// Write a-rate variables and arrays to the specified data set
//
// For sample accurate mode, get the offset and early variables
// Calculate the size of the incoming vector
// If the vector is 0 return, no more data to write
// Expand the dataset size by ksmps, because data is written in chunks
// For sample accurate mode the exact dataset size is set when writing is finished
// Write the data to the dataset
// Increment the offset by the size of the vector that was just written

void HDF5Write_writeAudioData(CSOUND *csound, HDF5Write *self, HDF5Dataset *dataset, MYFLT *dataPointer)
{
    size_t offset = self->h.insdshead->ksmps_offset;
    size_t early  = self->h.insdshead->ksmps_no_end;
    
    int vectorSize = (int)(self->ksmps - offset - early);
    
    if (vectorSize == 0) {
        return;
    }
    
    dataset->datasetSize[dataset->rank - 1] += self->ksmps;

    HDF5Write_writeData(csound, self, dataset, &dataPointer[offset]);
    
    dataset->offset[dataset->rank - 1] += vectorSize;
}

// Write k-rate variables and arrays to the specified data set
//
// Increment the data set size by 1
// Write the data to the dataset
// Increment the offset by 1

void HDF5Write_writeControlData(CSOUND *csound, HDF5Write *self, HDF5Dataset *dataset, MYFLT *dataPointer)
{
    dataset->datasetSize[dataset->rank - 1]++;
    
    HDF5Write_writeData(csound, self, dataset, dataPointer);
    
    dataset->offset[dataset->rank - 1]++;
}

// Send each input argument to the necessary writing function
//
// Iterate through the dataset array, select the current
// Depending on the dataset type send to the necessary write function

int HDF5Write_process(CSOUND *csound, HDF5Write *self)
{
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        HDF5Dataset *currentDataset = &self->datasets[i];

        switch (currentDataset->writeType) {
                
            case ARATE_ARRAY: {
                
                HDF5Write_writeAudioData(csound, self, currentDataset, ((ARRAYDAT *)currentDataset->argumentPointer)->data);
                break;
            }
            case KRATE_ARRAY: {
                
                HDF5Write_writeControlData(csound, self, currentDataset, ((ARRAYDAT *)currentDataset->argumentPointer)->data);
                break;
            }
            case ARATE_VAR: {
                
                HDF5Write_writeAudioData(csound, self, currentDataset, currentDataset->argumentPointer);
                break;
            }
            case KRATE_VAR: {
                
                HDF5Write_writeControlData(csound, self, currentDataset, currentDataset->argumentPointer);
                break;
            }
            default: {
                
                break;
            }
        }
    }
    return OK;
}

// Close the hdf5 file and set the a-rate dataset extents for sample accurate mode
//
// Check that the datasets exist
// Iterate through the datasets, if a-rate, set the size to be the same as current offset
// Set the set extent of the dataset to the size
// Close the datasets, then close the file

int HDF5Write_finish(CSOUND *csound, void *inReference)
{
    HDF5Write *self = inReference;
    
    if (self->datasets != NULL) {
        
        for (size_t i = 0; i < self->inputArgumentCount; ++i) {
            
            HDF5Dataset *dataset = &self->datasets[i];
            
            switch (dataset->writeType) {
                    
                case ARATE_ARRAY: {
                    
                    dataset->datasetSize[dataset->rank - 1] = dataset->offset[dataset->rank - 1];
                    HDF5ERROR(H5Dset_extent(dataset->datasetID, dataset->datasetSize));
                    break;
                }
                case ARATE_VAR: {
                    
                    dataset->datasetSize[dataset->rank - 1] = dataset->offset[dataset->rank - 1];
                    HDF5ERROR(H5Dset_extent(dataset->datasetID, dataset->datasetSize));
                    break;
                }
                default: {
                    
                    break;
                }
            }
            
            HDF5ERROR(H5Dclose(dataset->datasetID));
        }
    }
    
    HDF5ERROR(H5Fclose(self->hdf5File->fileHandle));

    return OK;
}

// Check that the correct types of inputs have been used in the opcode
//
// Get the argument type for the first input
// If it is not a string stop csound
// Iterate over the rest of the input arguments, if any are strings stop csound

void HDF5Write_checkArgumentSanity(CSOUND *csound, const HDF5Write *self)
{
    ArgumentType type = HDF5IO_getArgumentTypeFromArgument(csound, self->arguments[0]);
    
    if (type != STRING_VAR) {
        
        csound->Die(csound, "hdf5write: Error, first argument does not appear to be a string, exiting");
    }
    
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        type = HDF5IO_getArgumentTypeFromArgument(csound, self->arguments[i + 1]);
        
        if (type == STRING_VAR
            ||
            type == UNKNOWN) {
            
            csound->Die(csound, "hdf5write: Error, unable to identify type of argument %zd", i);
        }
    }
}

// Create, or delete and create again a dataset in an hdf5 file
//
// Check to see if the dataset exists
// If it exists delete it
// Create the data space, set the writing chunk size and the empty space fill value
// Create the data set in the data space and write the argument type as a string attribute

void HDF5Write_initialiseHDF5Dataset(CSOUND *csound, HDF5Write *self, HDF5Dataset *dataset)
{
    htri_t result = H5Lexists(self->hdf5File->fileHandle, dataset->datasetName, H5P_DEFAULT);
    
    if (result == 1) {
        
        HDF5ERROR(H5Ldelete(self->hdf5File->fileHandle, dataset->datasetName, H5P_DEFAULT));
    }
    
    hid_t dataspaceID = H5Screate_simple(dataset->rank, dataset->chunkDimensions, dataset->maxDimensions);
    HDF5ERROR(dataspaceID);
    hid_t cparams = H5Pcreate(H5P_DATASET_CREATE);
    HDF5ERROR(cparams);
    
    HDF5ERROR(H5Pset_chunk(cparams, dataset->rank, dataset->chunkDimensions));
    
    MYFLT zero = 0;
    
    HDF5ERROR(H5Pset_fill_value(cparams, self->hdf5File->floatSize, &zero));
    
    dataset->datasetID = H5Dcreate2(self->hdf5File->fileHandle, dataset->datasetName,
                                    self->hdf5File->floatSize, dataspaceID, H5P_DEFAULT, cparams, H5P_DEFAULT);
    HDF5ERROR(dataset->datasetID);
    HDF5IO_writeStringAttribute(csound, self->hdf5File, dataset, "Variable Type", typeStrings[dataset->writeType]);
    
}

// Set up the variables for writing an array dataset
//
// Get the array from the argument pointer
// If its an i-rate array the rank is copied, if not we add a dimension
// Allocate arrays for the chunk sizes, maximum sizes, data set sizes and offset sizes
// Copy the sizes from the input array variables to the allocated size arrays
// If it's an a-rate array set the last chunk dimension to ksmps, last max dimension to unlimited and dataset size to 0
// If it's a k-rate array set the last chunk dimension to 1 and last max dimension to unlimited
// If it's an i-rate array just return

void HDF5Write_newArrayDataset(CSOUND *csound, HDF5Write *self,
                               HDF5Dataset *dataset)
{
    ARRAYDAT *array = (ARRAYDAT *)dataset->argumentPointer;
    
    if (dataset->writeType == IRATE_ARRAY) {
        
        dataset->rank = array->dimensions;
    }
    else {
        
        dataset->rank = array->dimensions + 1;
    }
    
    csound->AuxAlloc(csound, dataset->rank * sizeof(hsize_t), &dataset->chunkDimensionsMemory);
    dataset->chunkDimensions = dataset->chunkDimensionsMemory.auxp;

    csound->AuxAlloc(csound, dataset->rank * sizeof(hsize_t), &dataset->maxDimensionsMemory);
    dataset->maxDimensions = dataset->maxDimensionsMemory.auxp;
    
    csound->AuxAlloc(csound, dataset->rank * sizeof(hsize_t), &dataset->datasetSizeMemory);
    dataset->datasetSize = dataset->datasetSizeMemory.auxp;

    csound->AuxAlloc(csound, dataset->rank * sizeof(hsize_t), &dataset->offsetMemory);
    dataset->offset = dataset->offsetMemory.auxp;
    
    for (size_t i = 0; i < array->dimensions; ++i) {
        
        dataset->chunkDimensions[i] = array->sizes[i];
        dataset->maxDimensions[i] = array->sizes[i];
        dataset->datasetSize[i] = array->sizes[i];
    }
    
    switch (dataset->writeType) {
            
        case ARATE_ARRAY: {
            
            dataset->chunkDimensions[dataset->rank - 1] = self->ksmps;
            dataset->maxDimensions[dataset->rank - 1] = H5S_UNLIMITED;
            dataset->datasetSize[dataset->rank - 1] = 0;

            break;
        }
        case KRATE_ARRAY: {
            
            dataset->chunkDimensions[dataset->rank - 1] = 1;
            dataset->maxDimensions[dataset->rank - 1] = H5S_UNLIMITED;
            break;
        }
        case IRATE_ARRAY: {
            
            return;
        }
        default: {
            
            csound->Die(csound, "This shouldn't happen, exiting");
            break;
        }
    }
}

// Set up variables for writing a scalar dataset
//
// Set the rank to 1
// Allocate memory for chunk sizes, maximum sizes, dataset sizes and offsets
// If the argument is not an i-rate variable:
//  Set the chunk dimensions to ksmps if a-rate, 1 if k-rate
//  Set maximum dimensions to unlimited
//  Set the data size to 0
// If it is an i-rate variable:
//  Set the data set size to 1
//  Set the chunk dimensions to 1
//  Set the maximum dimensions to 1
// Set the offset to 0

void HDF5Write_newScalarDataset(CSOUND *csound, HDF5Write *self,
                                HDF5Dataset *dataset)
{
    dataset->rank = 1;
    csound->AuxAlloc(csound, sizeof(hsize_t), &dataset->chunkDimensionsMemory);
    dataset->chunkDimensions = dataset->chunkDimensionsMemory.auxp;
    
    csound->AuxAlloc(csound, sizeof(hsize_t), &dataset->maxDimensionsMemory);
    dataset->maxDimensions = dataset->maxDimensionsMemory.auxp;
    
    csound->AuxAlloc(csound, sizeof(hsize_t), &dataset->datasetSizeMemory);
    dataset->datasetSize = dataset->datasetSizeMemory.auxp;
    
    csound->AuxAlloc(csound, sizeof(hsize_t), &dataset->offsetMemory);
    dataset->offset = dataset->offsetMemory.auxp;
    
    if (dataset->writeType != IRATE_VAR) {
        
        dataset->chunkDimensions[0] = dataset->writeType == ARATE_VAR ? self->ksmps : 1;
        dataset->maxDimensions[0] = H5S_UNLIMITED;
        dataset->datasetSize[0] = 0;
    }
    else {
        dataset->datasetSize[0] = 1;
        dataset->chunkDimensions[0] = 1;
        dataset->maxDimensions[0] = 1;
    }
    
    dataset->offset[0] = 0;
}

// Create the datasets for each argument to be written
//
// Allocate the memory for the datasets array
// Get the current empty dataset from the array
// Set it's dataset name as the input arguments name + 1 which is after the file path string
// Get the argument pointer from the arguments + 1 after the file path string
// Get the enum write type from the argument pointer
// Depending on the write type set up the variables in the correct way for writing during performance
// If the variables are i-rate set up the variables and write them

void HDF5Write_createDatasets(CSOUND *csound, HDF5Write *self)
{
    csound->AuxAlloc(csound, sizeof(HDF5Dataset) * self->inputArgumentCount, &self->datasetsMemory);
    self->datasets = self->datasetsMemory.auxp;
    
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        HDF5Dataset *currentDataset = &self->datasets[i];
        currentDataset->datasetName = csound->GetInputArgName(self, (int)i + 1);
        currentDataset->argumentPointer = self->arguments[i + 1];
        currentDataset->writeType = HDF5IO_getArgumentTypeFromArgument(csound, currentDataset->argumentPointer);
        
        switch (currentDataset->writeType) {
                
            case ARATE_ARRAY: {
                
                HDF5Write_newArrayDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                break;
            }
            case KRATE_ARRAY: {
                
                HDF5Write_newArrayDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                break;
            }
            case IRATE_ARRAY: {
                
                HDF5Write_newArrayDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                HDF5Write_writeData(csound, self, currentDataset,
                                    ((ARRAYDAT *)currentDataset->argumentPointer)->data);
                break;
            }
            case ARATE_VAR: {
                
                HDF5Write_newScalarDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                break;
            }
            case KRATE_VAR: {
                
                HDF5Write_newScalarDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                break;
            }
            case IRATE_VAR: {
                
                HDF5Write_newScalarDataset(csound, self, currentDataset);
                HDF5Write_initialiseHDF5Dataset(csound, self, currentDataset);
                HDF5Write_writeData(csound, self, currentDataset,
                                    currentDataset->argumentPointer);
                break;
            }
            default: {
                
                break;
            }
        }
    }
}

#pragma mark - HDF5Read -

// Open datasets in a hdf5 file so they can be read by the opcode
//
// Get the amount of samples in a control pass
// Get the amount of input arguments minus the file path argument
// Get the amount of output arguments
// Check that input == output arguments and input arguments are strings, output not strings
// Register the finish callback to close the hdf5 file when performance is finished
// Check csound is running in sample accurate mode
// Get the path string from the first argument
// Open the hdf5 file then open the hdf5 datasets

int HDF5Read_initialise(CSOUND *csound, HDF5Read *self)
{
    self->ksmps = csound->GetKsmps(csound);
    self->inputArgumentCount = self->INOCOUNT - 1;
    self->outputArgumentCount = self->OUTOCOUNT;
    HDF5Read_checkArgumentSanity(csound, self);
    csound->RegisterDeinitCallback(csound, self, HDF5Read_finish);
    self->isSampleAccurate = HDF5IO_getSampleAccurate(csound);
    STRINGDAT *path = (STRINGDAT *)self->arguments[self->outputArgumentCount];
    self->hdf5File = HDF5IO_newHDF5File(csound, &self->hdf5FileMemory, path, false);
    HDF5Read_openDatasets(csound, self);
    
    return OK;
}

// Copy data from the a-rate variable read sample buffer to the output array data
//
// This function is used to copy data from a sample buffer each hdf5 struct to an array data member,
// this is because in sample accurate mode the stride of the data read from the hdf5 file
// isn't correct and must be offset properly before it is written to the array data

void HDF5Read_copySampleBufferToArray(size_t channelCount, MYFLT *sampleBuffer, MYFLT *arrayData,
                                      size_t vectorSize, size_t offset, size_t ksmps)
{
    for (size_t channel = 0; channel < channelCount; ++channel) {
        
        memcpy(&arrayData[ksmps * channel + offset],
               &sampleBuffer[vectorSize * channel],
               sizeof(MYFLT) * vectorSize);
    }
}

// Read data from an hdf5 file dataset
//
// Get the specified file space
// Select the hyperslab at the specified offset and chunk dimension
// Create a memory space from where the data is read
// Read the data from the memory space
// Close the open file and memory spaces

void HDF5Read_readData(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset,
                            hsize_t *offset, hsize_t *chunkDimensions, MYFLT *dataPointer)
{
    hid_t filespace = H5Dget_space(dataset->datasetID);
    HDF5ERROR(filespace);
    HDF5ERROR(H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, chunkDimensions, NULL));
    hid_t memspace  = H5Screate_simple(dataset->rank, chunkDimensions, NULL);
    HDF5ERROR(memspace);
    HDF5ERROR(H5Dread(dataset->datasetID, self->hdf5File->floatSize, memspace, filespace, H5P_DEFAULT, dataPointer));
    HDF5ERROR(H5Sclose(filespace));
    HDF5ERROR(H5Sclose(memspace));
}

// Read data at audio rate from a hdf5 file dataset
//
// If the offset is larger than the size of the dataset there is no more data to read so return
// Get the offset and early variables and work out the size of data to read
// If the read vector size plus the offset is larger than the dataset size reduce the vector size accordingly
// If the vector size is less than the ksmps value, use the sample buffer to store read data so the stride can
// be corrected before writing it to the array data, if not just point directly to array data
// Create the chunk dimensions variable, set the required size
// Read data from the hdf5 file
// If the vector size is not equal to ksmps correct the stride of data
// Increment the offset by the vector size

void HDF5Read_readAudioData(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset, MYFLT *inputDataPointer)
{   
    if (dataset->offset[dataset->rank - 1] >= dataset->datasetSize[dataset->rank - 1]) {
        
        return;
    }

    size_t offset = self->h.insdshead->ksmps_offset;
    size_t early  = self->h.insdshead->ksmps_no_end;
    
    int vectorSize = (int)(self->ksmps - offset - early);
    
    if (vectorSize + dataset->offset[dataset->rank - 1] > dataset->datasetSize[dataset->rank - 1]) {
        
        vectorSize = (int)(dataset->datasetSize[dataset->rank - 1] - dataset->offset[dataset->rank - 1]);
    }
    
    MYFLT *dataPointer = vectorSize != self->ksmps ? dataset->sampleBuffer : inputDataPointer;

    hsize_t chunkDimensions[dataset->rank];
    memcpy(chunkDimensions, dataset->datasetSize, sizeof(hsize_t) * dataset->rank);
    chunkDimensions[dataset->rank - 1] = vectorSize;
 

    HDF5Read_readData(csound, self, dataset, dataset->offset, chunkDimensions, dataPointer);

    if (vectorSize != self->ksmps) {

        HDF5Read_copySampleBufferToArray(dataset->elementCount, dataset->sampleBuffer, inputDataPointer,
                                         vectorSize, offset, self->ksmps);
    }

    dataset->offset[dataset->rank - 1] += vectorSize;
    
}

// Read data at control rate from a hdf5 dataset
//
// If the offset of the dataset is larger than the data set size, no more data to read to return
// Create chunk dimension variable and set the appropriate size
// Read the data from the dataset
// Increment the offset variable

void HDF5Read_readControlData(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset, MYFLT *dataPointer)
{
    if (dataset->offset[dataset->rank - 1] >= dataset->datasetSize[dataset->rank - 1]) {
        
        return;
    }
    
    hsize_t chunkDimensions[dataset->rank];
    memcpy(chunkDimensions, dataset->datasetSize, sizeof(hsize_t) * (dataset->rank - 1));
    chunkDimensions[dataset->rank - 1] = 1;

    HDF5Read_readData(csound, self, dataset, dataset->offset, chunkDimensions, dataPointer);
    dataset->offset[dataset->rank - 1]++;
    
}

// Read dataset variables during performance time
//
// Iterate through each of the opened datasets,
// Depending on the dataset read type use the appropriate read function to read the data

int HDF5Read_process(CSOUND *csound, HDF5Read *self)
{
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        HDF5Dataset *dataset = &self->datasets[i];
        
        switch (dataset->readType) {
                
            case ARATE_ARRAY: {
                
                HDF5Read_readAudioData(csound, self, dataset, ((ARRAYDAT *)dataset->argumentPointer)->data);
                break;
            }
            case KRATE_ARRAY: {
                
                HDF5Read_readControlData(csound, self, dataset, ((ARRAYDAT *)dataset->argumentPointer)->data);
                break;
            }
            case ARATE_VAR: {
                
                HDF5Read_readAudioData(csound, self, dataset, dataset->argumentPointer);
                break;
            }
            case KRATE_VAR: {
                
                HDF5Read_readControlData(csound, self, dataset, dataset->argumentPointer);
                break;
            }
                
            default: {
                
                break;
            }
        }
    }
    return OK;
}

// Close the necessary variables when reading has finished
//
// Iterate through open datasets closing them in the hdf5 file
// Close the hdf5 file

int HDF5Read_finish(CSOUND *csound, void *inReference)
{
    HDF5Read *self = inReference;

    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        HDF5Dataset *dataset = &self->datasets[i];
        
        HDF5ERROR(H5Dclose(dataset->datasetID));
    }

    HDF5ERROR(H5Fclose(self->hdf5File->fileHandle));

    return OK;
}

// Check the input and output arguments for validity
//
// Check to see that if the amount of input arguments matches output arguments
// Check that the input arguments are strings, and the output arguments are not strings

void HDF5Read_checkArgumentSanity(CSOUND *csound, const HDF5Read *self)
{
    if (self->inputArgumentCount != self->outputArgumentCount) {
        
        if (self->inputArgumentCount > self->outputArgumentCount) {
            
            csound->Die(csound, "hdf5read: Error, more input arguments than output arguments, exiting");
        }
        else {
            
            csound->Die(csound, "hdf5read: Error, more output arguments than input arguments, exiting");
        }
    }
    
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        ArgumentType inputType = HDF5IO_getArgumentTypeFromArgument(csound, self->arguments[self->outputArgumentCount + i]);
        ArgumentType outputType = HDF5IO_getArgumentTypeFromArgument(csound, self->arguments[i]);
        
        if (inputType != STRING_VAR) {
            
            csound->Die(csound, "hdf5read: Error, input argument %zd does not appear to be a string, exiting", i + 1);
        }
        else if (inputType == UNKNOWN) {
            
            csound->Die(csound, "hdf5read: Error, input argument %zd type is unknown, exiting", i + 1);
        }
        
        if (outputType == STRING_VAR) {
            
            csound->Die(csound, "hdf5read: Error, output argument %zd appears to be a string, exiting", i + 1);
        }
        else if (outputType == UNKNOWN) {
            
            csound->Die(csound, "hdf5read: Error, output argument %zd type is unknown, exiting", i + 1);
        }
    }
}

// Open the specified dataset in the hdf5 file
//
// Check that the hdf5 dataset exists in the file, if it doesn't stop csound
// If it does then open it

void HDF5Read_initialiseHDF5Dataset(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset)
{
    htri_t result = H5Lexists(self->hdf5File->fileHandle, dataset->datasetName, H5P_DEFAULT);
    
    if (result <= 0) {
        
        csound->Die(csound, "hdf5read: Error, dataset does not exist or cannot be found in file");
    }
    
    dataset->datasetID = H5Dopen2(self->hdf5File->fileHandle, dataset->datasetName, H5P_DEFAULT);
    HDF5ERROR(dataset->datasetID);
}

// Check opcode read types are compatible with the types written to the hdf5 dataset
//
// Get the written type from the hdf5 file
// If the opcode read type for the dataset is an array and the write type is an array or a-rate or k-rate variable return
// If the opcode read type is an a-rate or k-rate variable and the write type is an a-rate or k-rate vairable return
// If the opcode read type is an i-reate variable and the write type is an i-rate variable return
// Otherwise stop csound

void HDF5Read_checkReadTypeSanity(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset)
{
    char attributeString[12] = {"UNKNOWN\0"};
    HDF5IO_readStringAttribute(csound, self->hdf5File, dataset->datasetName, attributeString);
    dataset->writeType = HDF5IO_getArgumentTypeFromString(csound, attributeString);
    
    if (dataset->readType == ARATE_ARRAY
        ||
        dataset->readType == KRATE_ARRAY
        ||
        dataset->readType == IRATE_ARRAY) {
        
        if (dataset->writeType == ARATE_ARRAY
            ||
            dataset->writeType == KRATE_ARRAY
            ||
            dataset->writeType == IRATE_ARRAY
            ||
            dataset->writeType == ARATE_VAR
            ||
            dataset->writeType == KRATE_VAR) {
            
            return;
        }
    }
    else if (dataset->readType == ARATE_VAR
             ||
             dataset->readType == KRATE_VAR) {
        
        if (dataset->writeType == ARATE_VAR
            ||
            dataset->writeType == KRATE_VAR) {
            
            return;
        }
    }
    else if (dataset->readType == IRATE_VAR) {
        
        if (dataset->writeType == IRATE_VAR) {
            
            return;
        }
    }
    else {
        
        csound->Die(csound, "hdf5read: Unable to read saved type of dataset, exiting");
    }
}

// Allocate the memory for an output array from the opcode
//
// Get the pointer to the output argument and cast to array
// Get the rank and allocate the dimension sizes memory
// Assign first dimension size to array sizes and dataset element count
// If the rank is greater than 1, multiply dimensions to get element count
// Allocate data space for the array using the element count

void HDF5Read_allocateArray(CSOUND *csound, HDF5Dataset *dataset,
                            hsize_t rank, hsize_t *dimensions)
{
    ARRAYDAT *array = dataset->argumentPointer;
    array->dimensions = (int)rank;
    csound->AuxAlloc(csound, sizeof(int) * rank, &dataset->arraySizesMemory);
    array->sizes = dataset->arraySizesMemory.auxp;
    
    array->sizes[0] = (int)dimensions[0];
    dataset->elementCount = dimensions[0];
    
    if (rank > 1) {
        
        for (size_t i = 1; i < rank; ++i) {
            
            array->sizes[i] = (int)dimensions[i];
            dataset->elementCount *= array->sizes[i];
        }
    }
    
    CS_VARIABLE *arrayVariable = array->arrayType->createVariable(csound, NULL);
    array->arrayMemberSize = arrayVariable->memBlockSize;
    csound->AuxAlloc(csound, arrayVariable->memBlockSize * dataset->elementCount, &dataset->arrayDataMemory);
    array->data = dataset->arrayDataMemory.auxp;
}


// Initialise the dataset and prepare for reading an a-rate, k-rate or i-rate array during performance time
//
// Get the data space from the dataset in the hdf5 file
// Get the rank from the data space
// Allocate the size array for specified rank
// Get the dimensions of the dataset and copy to dataset size array
// If requested output type for dataset is not an i-rate array:
// Create the dimensions variable and copy the dataset size minus the last dimension to the array
// Then allocate the array data for the output argument
// Allocate the memory for the offset variable
// If it's an a-rate array and sample accurate allocate data for the sample buffer
// Else if it's an i-rate array copy the array dimensions including the last one
// Then allocate the array data for the output argument
// Cast the argument pointer to an array, then read the data into the array from the hdf5 file

void HDF5Read_initialiseArrayOutput(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset)
{
    
    hid_t dataspaceID = H5Dget_space(dataset->datasetID);
    HDF5ERROR(dataspaceID);
    dataset->rank = H5Sget_simple_extent_ndims(dataspaceID);
    
    csound->AuxAlloc(csound, sizeof(hsize_t) * dataset->rank, &dataset->datasetSizeMemory);
    dataset->datasetSize = dataset->datasetSizeMemory.auxp;
    
    H5Sget_simple_extent_dims(dataspaceID, dataset->datasetSize, NULL);
    HDF5ERROR(H5Sclose(dataspaceID));
    
    if (dataset->readType != IRATE_ARRAY) {
        
        hsize_t arrayDimensions[dataset->rank - 1];
        memcpy(arrayDimensions, dataset->datasetSize, (dataset->rank - 1) * sizeof(hsize_t));
        HDF5Read_allocateArray(csound, dataset, (dataset->rank - 1), arrayDimensions);

        csound->AuxAlloc(csound, sizeof(hsize_t) * dataset->rank, &dataset->offsetMemory);
        dataset->offset = dataset->offsetMemory.auxp;
        
        if (dataset->readType == ARATE_ARRAY
            ||
            self->isSampleAccurate == true) {
            
            csound->AuxAlloc(csound, dataset->elementCount * self->ksmps * sizeof(MYFLT), &dataset->sampleBufferMemory);
            dataset->sampleBuffer = dataset->sampleBufferMemory.auxp;
        }
    }
    else {
        
        hsize_t arrayDimensions[dataset->rank];
        memcpy(arrayDimensions, dataset->datasetSize, dataset->rank * sizeof(hsize_t));
        HDF5Read_allocateArray(csound, dataset, dataset->rank, arrayDimensions);
        ARRAYDAT *array = dataset->argumentPointer;
        hsize_t offset[dataset->rank];
        memset(offset, 0, sizeof(hsize_t) * dataset->rank);
        HDF5Read_readData(csound, self, dataset, offset, arrayDimensions, array->data);
    }
}

// Initialise the dataset and prepare for reading an a-rate, k-rate or i-rate variable during performance time
//
// Get the data space from the dataset in the hdf5 file
// Set the rank as 1, the variable is a scalar
// If the dataset read type is a-rate or k-rate:
// Allocate the dataset size memory, get the size of the dataset and copy to the size memory
// Allocate offset memory and set to 0
// If the dataset is to be read at a-rate and we are running sample accurate allocate the sample buffer
// Otherwise create array dimesions variable, set to 1, create offset variable, set to 0 and read the i-rate variable

void HDF5Read_initialiseScalarOutput(CSOUND *csound, HDF5Read *self, HDF5Dataset *dataset)
{
    hid_t dataspaceID = H5Dget_space(dataset->datasetID);
    HDF5ERROR(dataspaceID);
    dataset->rank = 1;

    if (dataset->readType == ARATE_VAR
        ||
        dataset->readType == KRATE_VAR) {
        
        csound->AuxAlloc(csound, sizeof(hsize_t) * dataset->rank, &dataset->datasetSizeMemory);
        dataset->datasetSize = dataset->datasetSizeMemory.auxp;
        
        H5Sget_simple_extent_dims(dataspaceID, dataset->datasetSize, NULL);
        HDF5ERROR(H5Sclose(dataspaceID));
        
        csound->AuxAlloc(csound, sizeof(hsize_t), &dataset->offsetMemory);
        dataset->offset = dataset->offsetMemory.auxp;
        memset(dataset->offset, 0, sizeof(hsize_t));
        
        if (dataset->readType == ARATE_VAR
            &&
            self->isSampleAccurate == true) {
            
            csound->AuxAlloc(csound, self->ksmps * sizeof(MYFLT), &dataset->sampleBufferMemory);
            dataset->sampleBuffer = dataset->sampleBufferMemory.auxp;
            dataset->elementCount = 1;
        }
    } else {
        
        hsize_t arrayDimensions[1] = {1};
        hsize_t offset[1] = {0};
        HDF5Read_readData(csound, self, dataset, offset, arrayDimensions, dataset->argumentPointer);
    }
}

// Open the datasets in an hdf5 file for reading
//
// Allocate memory for the datasets array
// Iterate through the datasets in the array
// Assign the input argument name to the dataset name
// Get the read type from the arguments
// Assign the argument data pointer
// Check that the read type and write type are compatible
// Initialise the data set using the corresponding function for read type specified

void HDF5Read_openDatasets(CSOUND *csound, HDF5Read *self)
{
    csound->AuxAlloc(csound, sizeof(HDF5Dataset) * self->inputArgumentCount, &self->datasetsMemory);
    self->datasets = self->datasetsMemory.auxp;
    
    for (size_t i = 0; i < self->inputArgumentCount; ++i) {
        
        HDF5Dataset *currentDataset = &self->datasets[i];
        STRINGDAT *inputArgument = (STRINGDAT *)self->arguments[self->outputArgumentCount + i + 1];
        currentDataset->datasetName = inputArgument->data;
        currentDataset->readType = HDF5IO_getArgumentTypeFromArgument(csound, self->arguments[i]);
        currentDataset->argumentPointer = self->arguments[i];
        HDF5Read_checkReadTypeSanity(csound, self, currentDataset);
        HDF5Read_initialiseHDF5Dataset(csound, self, currentDataset);
        
        switch (currentDataset->readType) {
            case ARATE_ARRAY: {
                
                HDF5Read_initialiseArrayOutput(csound, self, currentDataset);
                break;
            }
            case KRATE_ARRAY: {
                
                HDF5Read_initialiseArrayOutput(csound, self, currentDataset);
                break;
            }
            case IRATE_ARRAY: {
                
                HDF5Read_initialiseArrayOutput(csound, self, currentDataset);
                break;
            }
            case ARATE_VAR: {
                
                HDF5Read_initialiseScalarOutput(csound, self, currentDataset);
                break;
            }
            case KRATE_VAR: {
                
                HDF5Read_initialiseScalarOutput(csound, self, currentDataset);
                break;
            }
            case IRATE_VAR: {
                
                HDF5Read_initialiseScalarOutput(csound, self, currentDataset);
                break;
            }
            default:
                break;
                
        }
    }
}


