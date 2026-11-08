#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifndef NO_SERIAL_OPCODES
#if defined(_WIN32) || defined(CSOUND_SERIAL_WINDOWS_TEST)
#include <winsock2.h>
#ifndef WIN32
#define WIN32
#endif
extern HANDLE WINAPI testCreateFileA(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
#define CreateFileA testCreateFileA
extern BOOL WINAPI testCloseHandle(HANDLE);
#define CloseHandle testCloseHandle
extern BOOL WINAPI testGetCommState(HANDLE, LPDCB);
#define GetCommState testGetCommState
extern BOOL WINAPI testSetCommState(HANDLE, LPDCB);
#define SetCommState testSetCommState
extern BOOL WINAPI testSetCommTimeouts(HANDLE, LPCOMMTIMEOUTS);
#define SetCommTimeouts testSetCommTimeouts
extern BOOL WINAPI testReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
#define ReadFile testReadFile
extern BOOL WINAPI testWriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
#define WriteFile testWriteFile
extern BOOL WINAPI testPurgeComm(HANDLE, DWORD);
#define PurgeComm testPurgeComm
#endif
#include "serial_test_opcode.h"
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/serial.c"
#endif
