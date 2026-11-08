#ifndef CSOUND_TEST_SERIAL_OPCODE_H
#define CSOUND_TEST_SERIAL_OPCODE_H
#include "../../../Opcodes/serial.h"
#ifdef __cplusplus
extern "C" {
#endif
#define serialport_init csound_test_serialport_init
int32_t serialport_init(CSOUND *, const char *, int32_t);
#define serialBegin csound_test_serialBegin
int32_t serialBegin(CSOUND *, SERIALBEGIN *);
#define serialEnd csound_test_serialEnd
int32_t serialEnd(CSOUND *, SERIALEND *);
#define serialWrite csound_test_serialWrite
int32_t serialWrite(CSOUND *, SERIALWRITE *);
#define serialWrite_S csound_test_serialWrite_S
int32_t serialWrite_S(CSOUND *, SERIALWRITE *);
#define serialRead csound_test_serialRead
int32_t serialRead(CSOUND *, SERIALREAD *);
#define serialPrint csound_test_serialPrint
int32_t serialPrint(CSOUND *, SERIALPRINT *);
#define serialFlush csound_test_serialFlush
int32_t serialFlush(CSOUND *, SERIALFLUSH *);
#define serialAvailable csound_test_serialAvailable
int32_t serialAvailable(CSOUND *, SERIALAVAIL *);
#define serialPeekByte csound_test_serialPeekByte
int32_t serialPeekByte(CSOUND *, SERIALPEEK *);
#define arduinoStart csound_test_arduinoStart
int32_t arduinoStart(CSOUND *, ARD_START *);
#define arduinoStop csound_test_arduinoStop
int32_t arduinoStop(CSOUND *, ARD_START *);
#define arduino_deinit csound_test_arduino_deinit
int32_t arduino_deinit(CSOUND *, ARD_START *);
#define arduinoReadSetup csound_test_arduinoReadSetup
int32_t arduinoReadSetup(CSOUND *, ARD_READ *);
#define arduinoRead csound_test_arduinoRead
int32_t arduinoRead(CSOUND *, ARD_READ *);
#define arduinoReadFSetup csound_test_arduinoReadFSetup
int32_t arduinoReadFSetup(CSOUND *, ARD_READF *);
#define arduinoReadF csound_test_arduinoReadF
int32_t arduinoReadF(CSOUND *, ARD_READF *);
#ifdef __cplusplus
}
#endif
#endif
