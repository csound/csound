#ifndef OPCLOAD_H_INCLUDED
#define OPCLOAD_H_INCLUDED
/*
*       Plug-in opcodes for Csound.
*       By Michael Gogins.
*       23 February 1997 
*/
#ifdef __cplusplus
extern "C" {
#endif
/*
*       Platform-dependent definitions and declarations.
*/
#ifdef WIN32
#define PUBLIC __declspec(dllexport) 
#define LIBRARY_CALL WINAPI
#else
#define PUBLIC
#define LIBRARY_CALL
#endif
/*
*       Return value preprocessor definitions.
*/
#define CS_SUCCESS                      0
#define CS_OPCODE_NOT_FOUND             1
#define CS_OPCODE_REGISTER_FAILED       2
#define CS_OUT_OF_MEMORY                3
/*
*       Signature for opcode registration function.
*       Both Csound and opcode must be compiled
*       with 8 byte structure member alignment.
*/
typedef PUBLIC long (csOpcodeRegisterType)
       (        /*   Used to iterate through all opcodes in a library. */
        long opcodeSubscript, /*   Csound audio sampling rate in Hz. */
        float *samplingRateIn,/*   Csound control sampling rate in Hz. */
        float *kontrolRateIn, /*   Control samples per audio sample. */
        int *audioSamplesPerKontrolSampleIn, /*   Channels in the soundfile. */
        int *channelCountIn,    /*   Address of the function table array in Csound. */
        FUNC *functionTablesIn[],/*   Address of the opcode's dispatch table entry,
                                      to be filled in by the opcode library. */
        OENTRY *opcodeEntryOut);
/*
 *      Platform-independent function 
 *      to load a shared library.
 */
long csLibraryLoad(const char *libraryPath);
/*
 *      Platform-independent function 
 *      to get a function address
 *      in a shared library.
 */
long csLibraryProcedureAddressGet(long library, const char *procedureName);
/*
*       Load an opcode from a shared library
*       and register its class in the opcode dispatch table.
*/
long csOpcodeLoad(const char *libraryPath);
/*
*       Load all opcodes in OPCODEDIR or ./OPCODES/*.OPC
*/
long csOpcodeLoadAll();
#ifdef __cplusplus
};
#endif
/*
*       OPCLOAD_H_INCLUDED
*/
#endif

